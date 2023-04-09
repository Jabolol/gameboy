#include "../include/gameboy.h"

static instruction_t *by_opcode(InstructionsClass *self, uint8_t opcode)
{
    return self->instructions + opcode;
}

static char *lookup(InstructionsClass *self, instruction_type_t instruction)
{
    return self->lookup_table[instruction];
}

static void proc_none(CPUClass __attribute__((unused)) * cpu)
{
    HANDLE_ERROR("invalid instruction");
}

static void proc_nop(CPUClass __attribute__((unused)) * cpuﬂ)
{
    return;
}

static void proc_di(CPUClass *cpu)
{
    cpu->context->int_master_enabled = false;
}

static void proc_ld(CPUClass __attribute__((unused)) * cpu)
{
    NOT_IMPLEMENTED();
}

static void proc_xor(CPUClass *cpu)
{
    cpu->context->registers.a ^= cpu->context->fetched_data & 0xFF;
    cpu->set_flags(cpu, cpu->context->registers.a == 0, 0, 0, 0);
}

static void proc_jp(CPUClass *cpu)
{
    if (cpu->check_condition(cpu)) {
        cpu->context->registers.pc = cpu->context->fetched_data;
        // TODO(jabolo): implement cycles
    }
}

static proc_fn get_proc(InstructionsClass *self, instruction_type_t type)
{
    return self->processors[type];
}

const InstructionsClass init_instructions = {
    {
        ._size = sizeof(InstructionsClass),
        ._name = "Instructions",
        ._constructor = NULL,
        ._destructor = NULL,
    },
    .instructions =
        {
            [0x00] = {IN_NOP, AM_IMP, 0, 0, 0, 0},
            [0x05] = {IN_DEC, AM_R, RT_B, 0, 0, 0},
            [0x0E] = {IN_LD, AM_R_D8, RT_C, 0, 0, 0},
            [0xAF] = {IN_XOR, AM_R, RT_A, 0, 0, 0},
            [0xC3] = {IN_JP, AM_D16, 0, 0, 0, 0},
            [0xF3] = {IN_DI, 0, 0, 0, 0, 0},
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
            [IN_JP] = proc_jp,
            [IN_DI] = proc_di,
            [IN_XOR] = proc_xor,
        },
    .by_opcode = by_opcode,
    .lookup = lookup,
    .get_proc = get_proc,
};

const class_t *Instructions = (const class_t *) &init_instructions;
