#include "../include/gameboy.h"

static instruction_t *by_opcode(InstructionsClass *self, uint8_t opcode)
{
    return self->instructions + opcode;
}

static char *lookup(InstructionsClass *self, instruction_type_t instruction)
{
    return self->lookup_table[instruction];
}

static void proc_none(CPUClass UNUSED *cpu)
{
    HANDLE_ERROR("invalid instruction");
}

static void proc_nop(CPUClass UNUSED *cpu)
{
    return;
}

static void proc_di(CPUClass *cpu)
{
    cpu->context->int_master_enabled = false;
}

static void proc_ld(CPUClass *cpu)
{
    if (cpu->context->dest_is_mem) {
        if (cpu->context->inst->register_2 >= RT_AF) {
            cpu->parent->cycles(cpu->parent, 1);
            cpu->bus->write16(
                cpu->bus, cpu->context->mem_dest, cpu->context->fetched_data);
        } else {
            cpu->bus->write(
                cpu->bus, cpu->context->mem_dest, cpu->context->fetched_data);
        }
        cpu->parent->cycles(cpu->parent, 1);
        return;
    }
    if (cpu->context->inst->mode == AM_HL_SPR) {
        uint8_t hflag =
            (cpu->read_register(cpu, cpu->context->inst->register_2) & 0xF)
                + (cpu->context->fetched_data & 0xF)
            >= 0x10;
        uint8_t cflag =
            (cpu->read_register(cpu, cpu->context->inst->register_2) & 0xFF)
                + (cpu->context->fetched_data & 0xFF)
            >= 0x100;
        uint16_t address =
            cpu->read_register(cpu, cpu->context->inst->register_2)
            + (char) cpu->context->fetched_data;
        cpu->set_register(cpu, cpu->context->inst->register_1, address);
        cpu->set_flags(cpu, 0, 0, hflag, cflag);
        return;
    }
    cpu->set_register(
        cpu, cpu->context->inst->register_1, cpu->context->fetched_data);
}

static void proc_ldh(CPUClass *cpu)
{
    if (cpu->context->inst->register_1 == RT_A) {
        cpu->set_register(cpu, cpu->context->inst->register_1,
            cpu->bus->read(cpu->bus, 0xFF00 | cpu->context->fetched_data));
    } else {
        cpu->bus->write(
            cpu->bus, cpu->context->mem_dest, cpu->context->registers.a);
    }
    cpu->parent->cycles(cpu->parent, 1);
}

static void proc_xor(CPUClass *cpu)
{
    cpu->context->registers.a ^= cpu->context->fetched_data & 0xFF;
    cpu->set_flags(cpu, cpu->context->registers.a == 0, 0, 0, 0);
}

static void jump(CPUClass *cpu, uint16_t address, bool push_pc)
{
    if (cpu->check_condition(cpu)) {
        if (push_pc) {
            cpu->parent->cycles(cpu->parent, 2);
            cpu->parent->stack->push16(
                cpu->parent->stack, cpu->get_registers(cpu)->pc);
        }
        cpu->get_registers(cpu)->pc = address;
        cpu->parent->cycles(cpu->parent, 1);
    }
}

static void proc_jp(CPUClass *cpu)
{
    jump(cpu, cpu->context->fetched_data, false);
}

static void proc_jr(CPUClass *cpu)
{
    char offset = (char) (cpu->context->fetched_data & 0xFF);
    uint16_t address = cpu->context->registers.pc + offset;
    jump(cpu, address, false);
}

static void proc_call(CPUClass *cpu)
{
    jump(cpu, cpu->context->fetched_data, true);
}

static void proc_rst(CPUClass *cpu)
{
    jump(cpu, cpu->context->inst->parameter, true);
}

static void proc_ret(CPUClass *cpu)
{
    if (cpu->context->inst->condition != CT_NONE) {
        cpu->parent->cycles(cpu->parent, 1);
    }
    if (cpu->check_condition(cpu)) {
        uint16_t lo = cpu->parent->stack->pop(cpu->parent->stack);
        cpu->parent->cycles(cpu->parent, 1);
        uint16_t hi = cpu->parent->stack->pop(cpu->parent->stack);
        cpu->parent->cycles(cpu->parent, 1);

        uint16_t n = (hi << 8) | lo;
        cpu->context->registers.pc = n;
        cpu->parent->cycles(cpu->parent, 1);
    }
}

static void proc_reti(CPUClass *cpu)
{
    cpu->context->int_master_enabled = true;
    proc_ret(cpu);
}

static void proc_pop(CPUClass *cpu)
{
    uint16_t lo = cpu->parent->stack->pop(cpu->parent->stack);
    cpu->parent->cycles(cpu->parent, 1);
    uint16_t hi = cpu->parent->stack->pop(cpu->parent->stack);
    cpu->parent->cycles(cpu->parent, 1);

    uint16_t n = (hi << 8) | lo;
    cpu->set_register(cpu, cpu->context->inst->register_1, n);

    if (cpu->context->inst->register_1 == RT_AF) {
        cpu->set_register(cpu, cpu->context->inst->register_1, n & 0xFFF0);
    }
}

static void proc_push(CPUClass *cpu)
{
    uint8_t hi =
        (cpu->read_register(cpu, cpu->context->inst->register_1) >> 8) & 0xFF;
    cpu->parent->cycles(cpu->parent, 1);
    cpu->parent->stack->push(cpu->parent->stack, hi);

    uint8_t lo =
        cpu->read_register(cpu, cpu->context->inst->register_2) & 0xFF;
    cpu->parent->cycles(cpu->parent, 1);
    cpu->parent->stack->push(cpu->parent->stack, lo);

    cpu->parent->cycles(cpu->parent, 1);
}

static proc_fn get_proc(InstructionsClass *self, instruction_type_t type)
{
    return self->processors[type];
}

const InstructionsClass init_instructions =
    {
        {
            ._size = sizeof(InstructionsClass),
            ._name = "Instructions",
            ._constructor = NULL,
            ._destructor = NULL,
        },
        // FIXME(jabolo): Initialize all fields
        .instructions =
            {
                [0x00] = {IN_NOP, AM_IMP},
                [0x01] = {IN_LD, AM_R_D16, RT_BC},
                [0x02] = {IN_LD, AM_MR_R, RT_BC, RT_A},
                [0x05] = {IN_DEC, AM_R, RT_B},
                [0x06] = {IN_LD, AM_R_D8, RT_B},
                [0x08] = {IN_LD, AM_A16_R, RT_NONE, RT_SP},
                [0x0A] = {IN_LD, AM_R_MR, RT_A, RT_BC},
                [0x0E] = {IN_LD, AM_R_D8, RT_C},
                [0x11] = {IN_LD, AM_R_D16, RT_DE},
                [0x12] = {IN_LD, AM_MR_R, RT_DE, RT_A},
                [0x15] = {IN_DEC, AM_R, RT_D},
                [0x16] = {IN_LD, AM_R_D8, RT_D},
                [0x18] = {IN_JR, AM_D8},
                [0x1A] = {IN_LD, AM_R_MR, RT_A, RT_DE},
                [0x1E] = {IN_LD, AM_R_D8, RT_E},
                [0x20] = {IN_JR, AM_D8, RT_NONE, RT_NONE, CT_NZ},
                [0x21] = {IN_LD, AM_R_D16, RT_HL},
                [0x22] = {IN_LD, AM_HLI_R, RT_HL, RT_A},
                [0x25] = {IN_DEC, AM_R, RT_H},
                [0x26] = {IN_LD, AM_R_D8, RT_H},
                [0x28] = {IN_JR, AM_D8, RT_NONE, RT_NONE, CT_Z},
                [0x2A] = {IN_LD, AM_R_HLI, RT_A, RT_HL},
                [0x2E] = {IN_LD, AM_R_D8, RT_L},
                [0x30] = {IN_JR, AM_D8, RT_NONE, RT_NONE, CT_NC},
                [0x31] = {IN_LD, AM_R_D16, RT_SP},
                [0x32] = {IN_LD, AM_HLD_R, RT_HL, RT_A},
                [0x35] = {IN_DEC, AM_R, RT_HL},
                [0x36] = {IN_LD, AM_MR_D8, RT_HL},
                [0x38] = {IN_JR, AM_D8, RT_NONE, RT_NONE, CT_C},
                [0x3A] = {IN_LD, AM_R_HLD, RT_A, RT_HL},
                [0x3E] = {IN_LD, AM_R_D8, RT_A},
                [0x40] = {IN_LD, AM_R_R, RT_B, RT_B},
                [0x41] = {IN_LD, AM_R_R, RT_B, RT_C},
                [0x42] = {IN_LD, AM_R_R, RT_B, RT_D},
                [0x43] = {IN_LD, AM_R_R, RT_B, RT_E},
                [0x44] = {IN_LD, AM_R_R, RT_B, RT_H},
                [0x45] = {IN_LD, AM_R_R, RT_B, RT_L},
                [0x46] = {IN_LD, AM_R_MR, RT_B, RT_HL},
                [0x47] = {IN_LD, AM_R_R, RT_B, RT_A},
                [0x48] = {IN_LD, AM_R_R, RT_C, RT_B},
                [0x49] = {IN_LD, AM_R_R, RT_C, RT_C},
                [0x4A] = {IN_LD, AM_R_R, RT_C, RT_D},
                [0x4B] = {IN_LD, AM_R_R, RT_C, RT_E},
                [0x4C] = {IN_LD, AM_R_R, RT_C, RT_H},
                [0x4D] = {IN_LD, AM_R_R, RT_C, RT_L},
                [0x4E] = {IN_LD, AM_R_MR, RT_C, RT_HL},
                [0x4F] = {IN_LD, AM_R_R, RT_C, RT_A},
                [0x50] = {IN_LD, AM_R_R, RT_D, RT_B},
                [0x51] = {IN_LD, AM_R_R, RT_D, RT_C},
                [0x52] = {IN_LD, AM_R_R, RT_D, RT_D},
                [0x53] = {IN_LD, AM_R_R, RT_D, RT_E},
                [0x54] = {IN_LD, AM_R_R, RT_D, RT_H},
                [0x55] = {IN_LD, AM_R_R, RT_D, RT_L},
                [0x56] = {IN_LD, AM_R_MR, RT_D, RT_HL},
                [0x57] = {IN_LD, AM_R_R, RT_D, RT_A},
                [0x58] = {IN_LD, AM_R_R, RT_E, RT_B},
                [0x59] = {IN_LD, AM_R_R, RT_E, RT_C},
                [0x5A] = {IN_LD, AM_R_R, RT_E, RT_D},
                [0x5B] = {IN_LD, AM_R_R, RT_E, RT_E},
                [0x5C] = {IN_LD, AM_R_R, RT_E, RT_H},
                [0x5D] = {IN_LD, AM_R_R, RT_E, RT_L},
                [0x5E] = {IN_LD, AM_R_MR, RT_E, RT_HL},
                [0x5F] = {IN_LD, AM_R_R, RT_E, RT_A},
                [0x60] = {IN_LD, AM_R_R, RT_H, RT_B},
                [0x61] = {IN_LD, AM_R_R, RT_H, RT_C},
                [0x62] = {IN_LD, AM_R_R, RT_H, RT_D},
                [0x63] = {IN_LD, AM_R_R, RT_H, RT_E},
                [0x64] = {IN_LD, AM_R_R, RT_H, RT_H},
                [0x65] = {IN_LD, AM_R_R, RT_H, RT_L},
                [0x66] = {IN_LD, AM_R_MR, RT_H, RT_HL},
                [0x67] = {IN_LD, AM_R_R, RT_H, RT_A},
                [0x68] = {IN_LD, AM_R_R, RT_L, RT_B},
                [0x69] = {IN_LD, AM_R_R, RT_L, RT_C},
                [0x6A] = {IN_LD, AM_R_R, RT_L, RT_D},
                [0x6B] = {IN_LD, AM_R_R, RT_L, RT_E},
                [0x6C] = {IN_LD, AM_R_R, RT_L, RT_H},
                [0x6D] = {IN_LD, AM_R_R, RT_L, RT_L},
                [0x6E] = {IN_LD, AM_R_MR, RT_L, RT_HL},
                [0x6F] = {IN_LD, AM_R_R, RT_L, RT_A},
                [0x70] = {IN_LD, AM_MR_R, RT_HL, RT_B},
                [0x71] = {IN_LD, AM_MR_R, RT_HL, RT_C},
                [0x72] = {IN_LD, AM_MR_R, RT_HL, RT_D},
                [0x73] = {IN_LD, AM_MR_R, RT_HL, RT_E},
                [0x74] = {IN_LD, AM_MR_R, RT_HL, RT_H},
                [0x75] = {IN_LD, AM_MR_R, RT_HL, RT_L},
                [0x76] = {IN_HALT},
                [0x77] = {IN_LD, AM_MR_R, RT_HL, RT_A},
                [0x78] = {IN_LD, AM_R_R, RT_A, RT_B},
                [0x79] = {IN_LD, AM_R_R, RT_A, RT_C},
                [0x7A] = {IN_LD, AM_R_R, RT_A, RT_D},
                [0x7B] = {IN_LD, AM_R_R, RT_A, RT_E},
                [0x7C] = {IN_LD, AM_R_R, RT_A, RT_H},
                [0x7D] = {IN_LD, AM_R_R, RT_A, RT_L},
                [0x7E] = {IN_LD, AM_R_MR, RT_A, RT_HL},
                [0x7F] = {IN_LD, AM_R_R, RT_A, RT_A},
                [0xAF] = {IN_XOR, AM_R, RT_A},
                [0xC0] = {IN_RET, AM_IMP, RT_NONE, RT_NONE, CT_NZ},
                [0xC1] = {IN_POP, AM_R, RT_BC},
                [0xC2] = {IN_JP, AM_D16, RT_NONE, RT_NONE, CT_NZ},
                [0xC3] = {IN_JP, AM_D16},
                [0xC4] = {IN_CALL, AM_D16, RT_NONE, RT_NONE, CT_NZ},
                [0xC5] = {IN_PUSH, AM_R, RT_BC},
                [0xC7] = {IN_RST, AM_IMP, RT_NONE, RT_NONE, CT_NONE, 0x00},
                [0xC8] = {IN_RET, AM_IMP, RT_NONE, RT_NONE, CT_Z},
                [0xC9] = {IN_RET},
                [0xCA] = {IN_JP, AM_D16, RT_NONE, RT_NONE, CT_Z},
                [0xCC] = {IN_CALL, AM_D16, RT_NONE, RT_NONE, CT_Z},
                [0xCD] = {IN_CALL, AM_D16},
                [0xCF] = {IN_RST, AM_IMP, RT_NONE, RT_NONE, CT_NONE, 0x08},
                [0xD0] = {IN_RET, AM_IMP, RT_NONE, RT_NONE, CT_NC},
                [0xD1] = {IN_POP, AM_R, RT_DE},
                [0xD2] = {IN_JP, AM_D16, RT_NONE, RT_NONE, CT_NC},
                [0xD4] = {IN_CALL, AM_D16, RT_NONE, RT_NONE, CT_NC},
                [0xD5] = {IN_PUSH, AM_R, RT_DE},
                [0xD7] = {IN_RST, AM_IMP, RT_NONE, RT_NONE, CT_NONE, 0x10},
                [0xD8] = {IN_RET, AM_IMP, RT_NONE, RT_NONE, CT_C},
                [0xD9] = {IN_RETI},
                [0xDA] = {IN_JP, AM_D16, RT_NONE, RT_NONE, CT_C},
                [0xDC] = {IN_CALL, AM_D16, RT_NONE, RT_NONE, CT_C},
                [0xDF] = {IN_RST, AM_IMP, RT_NONE, RT_NONE, CT_NONE, 0x18},
                [0xE0] = {IN_LDH, AM_A8_R, RT_NONE, RT_A},
                [0xE1] = {IN_POP, AM_R, RT_HL},
                [0xE2] = {IN_LD, AM_MR_R, RT_C, RT_A},
                [0xE5] = {IN_PUSH, AM_R, RT_HL},
                [0xE7] = {IN_RST, AM_IMP, RT_NONE, RT_NONE, CT_NONE, 0x20},
                [0xE9] = {IN_JP, AM_MR, RT_HL},
                [0xEA] = {IN_LD, AM_A16_R, RT_NONE, RT_A},
                [0xEF] = {IN_RST, AM_IMP, RT_NONE, RT_NONE, CT_NONE, 0x28},
                [0xF0] = {IN_LDH, AM_R_A8, RT_A},
                [0xF1] = {IN_POP, AM_R, RT_AF},
                [0xF2] = {IN_LD, AM_R_MR, RT_A, RT_C},
                [0xF3] = {IN_DI},
                [0xF5] = {IN_PUSH, AM_R, RT_AF},
                [0xF7] = {IN_RST, AM_IMP, RT_NONE, RT_NONE, CT_NONE, 0x30},
                [0xFA] = {IN_LD, AM_R_A16, RT_A},
                [0xFF] = {IN_RST, AM_IMP, RT_NONE, RT_NONE, CT_NONE, 0x38},
            },
        .lookup_table =
            {
                "<NONE>",
                "NOP",
                "LD",
                "INC",
                "DEC",
                "RLCA",
                "ADD",
                "RRCA",
                "STOP",
                "RLA",
                "JR",
                "RRA",
                "DAA",
                "CPL",
                "SCF",
                "CCF",
                "HALT",
                "ADC",
                "SUB",
                "SBC",
                "AND",
                "XOR",
                "OR",
                "CP",
                "POP",
                "JP",
                "PUSH",
                "RET",
                "CB",
                "CALL",
                "RETI",
                "LDH",
                "JPHL",
                "DI",
                "EI",
                "RST",
                "IN_ERR",
                "IN_RLC",
                "IN_RRC",
                "IN_RL",
                "IN_RR",
                "IN_SLA",
                "IN_SRA",
                "IN_SWAP",
                "IN_SRL",
                "IN_BIT",
                "IN_RES",
                "IN_SET",
            },
        .processors =
            {
                [IN_NONE] = proc_none,
                [IN_NOP] = proc_nop,
                [IN_LD] = proc_ld,
                [IN_LDH] = proc_ldh,
                [IN_JP] = proc_jp,
                [IN_DI] = proc_di,
                [IN_POP] = proc_pop,
                [IN_PUSH] = proc_push,
                [IN_JR] = proc_jr,
                [IN_CALL] = proc_call,
                [IN_RET] = proc_ret,
                [IN_RST] = proc_rst,
                [IN_RETI] = proc_reti,
                [IN_XOR] = proc_xor,
            },
        .by_opcode = by_opcode,
        .lookup = lookup,
        .get_proc = get_proc,
};

const class_t *Instructions = (const class_t *) &init_instructions;
