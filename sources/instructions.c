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

static void proc_ei(CPUClass *cpu)
{
    cpu->context->enabling_ime = true;
}

static void proc_ld(CPUClass *cpu)
{
    if (cpu->context->dest_is_mem) {
        if (cpu->is_16bit(cpu->context->inst->register_2)) {
            cpu->parent->cycles(cpu->parent, 1);
            cpu->parent->bus->write16(cpu->parent->bus, cpu->context->mem_dest,
                cpu->context->fetched_data);
        } else {
            cpu->parent->bus->write(cpu->parent->bus, cpu->context->mem_dest,
                cpu->context->fetched_data);
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
            cpu->parent->bus->read(
                cpu->parent->bus, 0xFF00 | cpu->context->fetched_data));
    } else {
        cpu->parent->bus->write(cpu->parent->bus, cpu->context->mem_dest,
            cpu->context->registers.a);
    }
    cpu->parent->cycles(cpu->parent, 1);
}

static void proc_rlca(CPUClass *cpu)
{
    uint8_t u = cpu->context->registers.a;
    bool c = (u >> 7) & 1;

    u = (u << 1) | c;
    cpu->context->registers.a = u;
    cpu->set_flags(cpu, 0, 0, 0, c);
}

static void proc_rrca(CPUClass *cpu)
{
    uint8_t b = cpu->context->registers.a & 1;

    cpu->context->registers.a >>= 1;
    cpu->context->registers.a |= (b << 7);
    cpu->set_flags(cpu, 0, 0, 0, b);
}

static void proc_rla(CPUClass *cpu)
{
    uint8_t u = cpu->context->registers.a;
    uint8_t cf = CPU_FLAG_C;
    uint8_t c = (u >> 7) & 1;

    cpu->context->registers.a = (u << 1) | cf;
    cpu->set_flags(cpu, 0, 0, 0, c);
}

static void proc_stop(UNUSED CPUClass *cpu)
{
    LOG("Stopping the CPU");
}

static void proc_daa(CPUClass *cpu)
{
    uint8_t u = 0;
    int32_t fc = 0;

    if (CPU_FLAG_H || (!CPU_FLAG_N && (cpu->context->registers.a & 0xF) > 9)) {
        u = 6;
    }
    if (CPU_FLAG_C || (!CPU_FLAG_N && cpu->context->registers.a > 0x99)) {
        u |= 0x60;
        fc = 1;
    }
    cpu->context->registers.a += CPU_FLAG_N ? -u : u;
    cpu->set_flags(cpu, cpu->context->registers.a == 0, -1, 0, fc);
}

static void proc_cpl(CPUClass *cpu)
{
    cpu->context->registers.a = ~cpu->context->registers.a;
    cpu->set_flags(cpu, -1, 1, 1, -1);
}

static void proc_scf(CPUClass *cpu)
{
    cpu->set_flags(cpu, -1, 0, 0, 1);
}

static void proc_ccf(CPUClass *cpu)
{
    cpu->set_flags(cpu, -1, 0, 0, CPU_FLAG_C ^ 1);
}

static void proc_halt(CPUClass *cpu)
{
    cpu->context->halted = true;
}

static void proc_rra(CPUClass *cpu)
{
    uint8_t carry = CPU_FLAG_C;
    uint8_t new_c = cpu->context->registers.a & 1;

    cpu->context->registers.a >>= 1;
    cpu->context->registers.a |= (carry << 7);
    cpu->set_flags(cpu, 0, 0, 0, new_c);
}

static void proc_and(CPUClass *cpu)
{
    cpu->context->registers.a &= cpu->context->fetched_data;
    cpu->set_flags(cpu, cpu->context->registers.a == 0, 0, 1, 0);
}

static void proc_or(CPUClass *cpu)
{
    cpu->context->registers.a |= cpu->context->fetched_data & 0xFF;
    cpu->set_flags(cpu, cpu->context->registers.a == 0, 0, 0, 0);
}

static void proc_cp(CPUClass *cpu)
{
    int32_t n = (int32_t) cpu->context->registers.a
        - (int32_t) cpu->context->fetched_data;

    cpu->set_flags(cpu, n == 0, 1,
        ((int32_t) cpu->context->registers.a & 0x0F)
                - ((int32_t) cpu->context->fetched_data & 0x0F)
            < 0,
        n < 0);
}

static void proc_cb(CPUClass *cpu)
{
    uint8_t op = cpu->context->fetched_data;
    register_type_t reg = cpu->decode_register(cpu, op & 0b111);
    uint8_t bit = (op >> 3) & 0b111;
    uint8_t bit_op = (op >> 6) & 0b11;
    uint8_t reg_val = cpu->read_register8(cpu, reg);

    cpu->parent->cycles(cpu->parent, 1);
    if (reg == RT_HL) {
        cpu->parent->cycles(cpu->parent, 2);
    }
    switch (bit_op) {
        case CB_BIT: {
            return cpu->set_flags(cpu, !(reg_val & (1 << bit)), 0, 1, -1);
        }
        case CB_RST: {
            reg_val &= ~(1 << bit);
            return cpu->set_register8(cpu, reg, reg_val);
        }
        case CB_SET: {
            reg_val |= (1 << bit);
            return cpu->set_register8(cpu, reg, reg_val);
        }
    }

    bool flag_c = CPU_FLAG_C;

    switch (bit) {
        case CB_RLC: {
            bool set_c = false;
            uint8_t result = (reg_val << 1) & 0xFF;

            if ((reg_val & (1 << 7)) != 0) {
                result |= 1;
                set_c = true;
            }
            cpu->set_register8(cpu, reg, result);
            cpu->set_flags(cpu, result == 0, false, false, set_c);
            break;
        }
        case CB_RRC: {
            uint8_t old = reg_val;

            reg_val >>= 1;
            reg_val |= (old << 7);
            cpu->set_register8(cpu, reg, reg_val);
            cpu->set_flags(cpu, !reg_val, false, false, old & 1);
            break;
        }
        case CB_RL: {
            uint8_t old = reg_val;

            reg_val <<= 1;
            reg_val |= flag_c;
            cpu->set_register8(cpu, reg, reg_val);
            cpu->set_flags(cpu, !reg_val, false, false, !!(old & 0x80));
            break;
        }
        case CB_RR: {
            uint8_t old = reg_val;

            reg_val >>= 1;
            reg_val |= (flag_c << 7);
            cpu->set_register8(cpu, reg, reg_val);
            cpu->set_flags(cpu, !reg_val, false, false, old & 1);
            break;
        }
        case CB_SLA: {
            uint8_t old = reg_val;

            reg_val <<= 1;
            cpu->set_register8(cpu, reg, reg_val);
            cpu->set_flags(cpu, !reg_val, false, false, !!(old & 0x80));
            break;
        }
        case CB_SRA: {
            uint8_t u = (int8_t) reg_val >> 1;

            cpu->set_register8(cpu, reg, u);
            cpu->set_flags(cpu, !u, 0, 0, reg_val & 1);
            break;
        }
        case CB_SWP: {
            reg_val = ((reg_val & 0xF0) >> 4) | ((reg_val & 0xF) << 4);
            cpu->set_register8(cpu, reg, reg_val);
            cpu->set_flags(cpu, reg_val == 0, false, false, false);
            break;
        }
        case CB_SRL: {
            uint8_t u = reg_val >> 1;
            cpu->set_register8(cpu, reg, u);
            cpu->set_flags(cpu, !u, 0, 0, reg_val & 1);
            break;
        }
    }
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
        cpu->read_register(cpu, cpu->context->inst->register_1) & 0xFF;
    cpu->parent->cycles(cpu->parent, 1);
    cpu->parent->stack->push(cpu->parent->stack, lo);

    cpu->parent->cycles(cpu->parent, 1);
}

static void proc_inc(CPUClass *cpu)
{
    uint16_t value =
        cpu->read_register(cpu, cpu->context->inst->register_1) + 1;

    if (cpu->is_16bit(cpu->context->inst->register_1)) {
        cpu->parent->cycles(cpu->parent, 1);
    }
    if (cpu->context->inst->register_1 == RT_HL
        && cpu->context->inst->mode == AM_MR) {
        value = cpu->parent->bus->read(
                    cpu->parent->bus, cpu->read_register(cpu, RT_HL))
            + 1;
        value &= 0xFF;
        cpu->parent->bus->write(
            cpu->parent->bus, cpu->read_register(cpu, RT_HL), value);
    } else {
        cpu->set_register(cpu, cpu->context->inst->register_1, value);
        value = cpu->read_register(cpu, cpu->context->inst->register_1);
    }
    if ((cpu->context->opcode & 0x03) == 0x03) {
        return;
    }
    cpu->set_flags(cpu, value == 0, 0, (value & 0x0F) == 0, -1);
}

static void proc_dec(CPUClass *cpu)
{
    uint16_t value =
        cpu->read_register(cpu, cpu->context->inst->register_1) - 1;

    if (cpu->is_16bit(cpu->context->inst->register_1)) {
        cpu->parent->cycles(cpu->parent, 1);
    }
    if (cpu->context->inst->register_1 == RT_HL
        && cpu->context->inst->mode == AM_MR) {
        value = cpu->parent->bus->read(
                    cpu->parent->bus, cpu->read_register(cpu, RT_HL))
            - 1;
        cpu->parent->bus->write(
            cpu->parent->bus, cpu->read_register(cpu, RT_HL), value);
    } else {
        cpu->set_register(cpu, cpu->context->inst->register_1, value);
        value = cpu->read_register(cpu, cpu->context->inst->register_1);
    }
    if ((cpu->context->opcode & 0x0B) == 0x0B) {
        return;
    }
    cpu->set_flags(cpu, value == 0, 1, (value & 0x0F) == 0x0F, -1);
}

static void proc_add(CPUClass *cpu)
{
    uint32_t value = cpu->read_register(cpu, cpu->context->inst->register_1)
        + cpu->context->fetched_data;
    bool is_16bit = cpu->is_16bit(cpu->context->inst->register_1);

    if (is_16bit) {
        cpu->parent->cycles(cpu->parent, 1);
    }
    if (cpu->context->inst->register_1 == RT_SP) {
        value = cpu->read_register(cpu, cpu->context->inst->register_1)
            + (char) cpu->context->fetched_data;
    }

    int32_t z = (value & 0XFF) == 0;
    int32_t h = (cpu->read_register(cpu, cpu->context->inst->register_1) & 0xF)
            + (cpu->context->fetched_data & 0xF)
        >= 0x10;
    int32_t c =
        (int32_t) (cpu->read_register(cpu, cpu->context->inst->register_1)
            & 0xFF)
            + (int32_t) (cpu->context->fetched_data & 0xFF)
        >= 0x100;

    if (is_16bit) {
        z = -1;
        h = (cpu->read_register(cpu, cpu->context->inst->register_1) & 0xFFF)
                + (cpu->context->fetched_data & 0xFFF)
            >= 0x1000;
        uint32_t n = ((uint32_t) cpu->read_register(
                         cpu, cpu->context->inst->register_1))
            + ((uint32_t) cpu->context->fetched_data);
        c = n >= 0x10000;
    }
    if (cpu->context->inst->register_1 == RT_SP) {
        z = 0;
        h = (cpu->read_register(cpu, cpu->context->inst->register_1) & 0xF)
                + (cpu->context->fetched_data & 0xF)
            >= 0x10;
        c = (int32_t) (cpu->read_register(cpu, cpu->context->inst->register_1)
                & 0xFF)
                + (int32_t) (cpu->context->fetched_data & 0xFF)
            >= 0x100;
    }
    cpu->set_register(cpu, cpu->context->inst->register_1, value & 0xFFFF);
    cpu->set_flags(cpu, z, 0, h, c);
}

static void proc_adc(CPUClass *cpu)
{
    uint16_t u = cpu->context->fetched_data;
    uint16_t a = cpu->context->registers.a;
    uint16_t c = CPU_FLAG_C;

    cpu->context->registers.a = (a + u + c) & 0xFF;
    cpu->set_flags(cpu, cpu->context->registers.a == 0, 0,
        (a & 0xF) + (u & 0xF) + c > 0xF, a + u + c > 0xFF);
}

static void proc_sub(CPUClass *cpu)
{
    uint16_t value = cpu->read_register(cpu, cpu->context->inst->register_1)
        - cpu->context->fetched_data;
    int32_t z = value == 0;
    int32_t h =
        ((int32_t) cpu->read_register(cpu, cpu->context->inst->register_1)
            & 0xF)
            - ((int32_t) cpu->context->fetched_data & 0xF)
        < 0;
    int32_t c =
        ((int32_t) cpu->read_register(cpu, cpu->context->inst->register_1))
            - ((int32_t) cpu->context->fetched_data)
        < 0;

    cpu->set_register(cpu, cpu->context->inst->register_1, value);
    cpu->set_flags(cpu, z, 1, h, c);
}

static void proc_sbc(CPUClass *cpu)
{
    uint8_t value = cpu->context->fetched_data + CPU_FLAG_C;
    int32_t z =
        (cpu->read_register(cpu, cpu->context->inst->register_1) - value) == 0;
    int32_t h =
        ((int32_t) cpu->read_register(cpu, cpu->context->inst->register_1)
            & 0xF)
            - ((int32_t) cpu->context->fetched_data & 0xF)
            - ((int32_t) CPU_FLAG_C)
        < 0;
    int32_t c =
        ((int32_t) cpu->read_register(cpu, cpu->context->inst->register_1))
            - ((int32_t) cpu->context->fetched_data) - ((int32_t) CPU_FLAG_C)
        < 0;

    cpu->set_register(cpu, cpu->context->inst->register_1,
        cpu->read_register(cpu, cpu->context->inst->register_1) - value);
    cpu->set_flags(cpu, z, 1, h, c);
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
                [0x03] = {IN_INC, AM_R, RT_BC},
                [0x04] = {IN_INC, AM_R, RT_B},
                [0x05] = {IN_DEC, AM_R, RT_B},
                [0x06] = {IN_LD, AM_R_D8, RT_B},
                [0x07] = {IN_RLCA},
                [0x08] = {IN_LD, AM_A16_R, RT_NONE, RT_SP},
                [0x09] = {IN_ADD, AM_R_R, RT_HL, RT_BC},
                [0x0A] = {IN_LD, AM_R_MR, RT_A, RT_BC},
                [0x0B] = {IN_DEC, AM_R, RT_BC},
                [0x0C] = {IN_INC, AM_R, RT_C},
                [0x0D] = {IN_DEC, AM_R, RT_C},
                [0x0E] = {IN_LD, AM_R_D8, RT_C},
                [0x0F] = {IN_RRCA},
                [0x10] = {IN_STOP},
                [0x11] = {IN_LD, AM_R_D16, RT_DE},
                [0x12] = {IN_LD, AM_MR_R, RT_DE, RT_A},
                [0x13] = {IN_INC, AM_R, RT_DE},
                [0x14] = {IN_INC, AM_R, RT_D},
                [0x15] = {IN_DEC, AM_R, RT_D},
                [0x16] = {IN_LD, AM_R_D8, RT_D},
                [0x17] = {IN_RLA},
                [0x18] = {IN_JR, AM_D8},
                [0x19] = {IN_ADD, AM_R_R, RT_HL, RT_DE},
                [0x1A] = {IN_LD, AM_R_MR, RT_A, RT_DE},
                [0x1B] = {IN_DEC, AM_R, RT_DE},
                [0x1C] = {IN_INC, AM_R, RT_E},
                [0x1D] = {IN_DEC, AM_R, RT_E},
                [0x1E] = {IN_LD, AM_R_D8, RT_E},
                [0x1F] = {IN_RRA},
                [0x20] = {IN_JR, AM_D8, RT_NONE, RT_NONE, CT_NZ},
                [0x21] = {IN_LD, AM_R_D16, RT_HL},
                [0x22] = {IN_LD, AM_HLI_R, RT_HL, RT_A},
                [0x23] = {IN_INC, AM_R, RT_HL},
                [0x24] = {IN_INC, AM_R, RT_H},
                [0x25] = {IN_DEC, AM_R, RT_H},
                [0x26] = {IN_LD, AM_R_D8, RT_H},
                [0x27] = {IN_DAA},
                [0x28] = {IN_JR, AM_D8, RT_NONE, RT_NONE, CT_Z},
                [0x29] = {IN_ADD, AM_R_R, RT_HL, RT_HL},
                [0x2A] = {IN_LD, AM_R_HLI, RT_A, RT_HL},
                [0x2B] = {IN_DEC, AM_R, RT_HL},
                [0x2C] = {IN_INC, AM_R, RT_L},
                [0x2D] = {IN_DEC, AM_R, RT_L},
                [0x2E] = {IN_LD, AM_R_D8, RT_L},
                [0x2F] = {IN_CPL},
                [0x30] = {IN_JR, AM_D8, RT_NONE, RT_NONE, CT_NC},
                [0x31] = {IN_LD, AM_R_D16, RT_SP},
                [0x32] = {IN_LD, AM_HLD_R, RT_HL, RT_A},
                [0x33] = {IN_INC, AM_R, RT_SP},
                [0x34] = {IN_INC, AM_MR, RT_HL},
                [0x35] = {IN_DEC, AM_MR, RT_HL},
                [0x36] = {IN_LD, AM_MR_D8, RT_HL},
                [0x37] = {IN_SCF},
                [0x38] = {IN_JR, AM_D8, RT_NONE, RT_NONE, CT_C},
                [0x39] = {IN_ADD, AM_R_R, RT_HL, RT_SP},
                [0x3A] = {IN_LD, AM_R_HLD, RT_A, RT_HL},
                [0x3B] = {IN_DEC, AM_R, RT_SP},
                [0x3C] = {IN_INC, AM_R, RT_A},
                [0x3D] = {IN_DEC, AM_R, RT_A},
                [0x3E] = {IN_LD, AM_R_D8, RT_A},
                [0x3F] = {IN_CCF},
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
                [0x80] = {IN_ADD, AM_R_R, RT_A, RT_B},
                [0x81] = {IN_ADD, AM_R_R, RT_A, RT_C},
                [0x82] = {IN_ADD, AM_R_R, RT_A, RT_D},
                [0x83] = {IN_ADD, AM_R_R, RT_A, RT_E},
                [0x84] = {IN_ADD, AM_R_R, RT_A, RT_H},
                [0x85] = {IN_ADD, AM_R_R, RT_A, RT_L},
                [0x86] = {IN_ADD, AM_R_MR, RT_A, RT_HL},
                [0x87] = {IN_ADD, AM_R_R, RT_A, RT_A},
                [0x88] = {IN_ADC, AM_R_R, RT_A, RT_B},
                [0x89] = {IN_ADC, AM_R_R, RT_A, RT_C},
                [0x8A] = {IN_ADC, AM_R_R, RT_A, RT_D},
                [0x8B] = {IN_ADC, AM_R_R, RT_A, RT_E},
                [0x8C] = {IN_ADC, AM_R_R, RT_A, RT_H},
                [0x8D] = {IN_ADC, AM_R_R, RT_A, RT_L},
                [0x8E] = {IN_ADC, AM_R_MR, RT_A, RT_HL},
                [0x8F] = {IN_ADC, AM_R_R, RT_A, RT_A},
                [0x90] = {IN_SUB, AM_R_R, RT_A, RT_B},
                [0x91] = {IN_SUB, AM_R_R, RT_A, RT_C},
                [0x92] = {IN_SUB, AM_R_R, RT_A, RT_D},
                [0x93] = {IN_SUB, AM_R_R, RT_A, RT_E},
                [0x94] = {IN_SUB, AM_R_R, RT_A, RT_H},
                [0x95] = {IN_SUB, AM_R_R, RT_A, RT_L},
                [0x96] = {IN_SUB, AM_R_MR, RT_A, RT_HL},
                [0x97] = {IN_SUB, AM_R_R, RT_A, RT_A},
                [0x98] = {IN_SBC, AM_R_R, RT_A, RT_B},
                [0x99] = {IN_SBC, AM_R_R, RT_A, RT_C},
                [0x9A] = {IN_SBC, AM_R_R, RT_A, RT_D},
                [0x9B] = {IN_SBC, AM_R_R, RT_A, RT_E},
                [0x9C] = {IN_SBC, AM_R_R, RT_A, RT_H},
                [0x9D] = {IN_SBC, AM_R_R, RT_A, RT_L},
                [0x9E] = {IN_SBC, AM_R_MR, RT_A, RT_HL},
                [0x9F] = {IN_SBC, AM_R_R, RT_A, RT_A},
                [0xA0] = {IN_AND, AM_R_R, RT_A, RT_B},
                [0xA1] = {IN_AND, AM_R_R, RT_A, RT_C},
                [0xA2] = {IN_AND, AM_R_R, RT_A, RT_D},
                [0xA3] = {IN_AND, AM_R_R, RT_A, RT_E},
                [0xA4] = {IN_AND, AM_R_R, RT_A, RT_H},
                [0xA5] = {IN_AND, AM_R_R, RT_A, RT_L},
                [0xA6] = {IN_AND, AM_R_MR, RT_A, RT_HL},
                [0xA7] = {IN_AND, AM_R_R, RT_A, RT_A},
                [0xA8] = {IN_XOR, AM_R_R, RT_A, RT_B},
                [0xA9] = {IN_XOR, AM_R_R, RT_A, RT_C},
                [0xAA] = {IN_XOR, AM_R_R, RT_A, RT_D},
                [0xAB] = {IN_XOR, AM_R_R, RT_A, RT_E},
                [0xAC] = {IN_XOR, AM_R_R, RT_A, RT_H},
                [0xAD] = {IN_XOR, AM_R_R, RT_A, RT_L},
                [0xAE] = {IN_XOR, AM_R_MR, RT_A, RT_HL},
                [0xAF] = {IN_XOR, AM_R_R, RT_A, RT_A},
                [0xB0] = {IN_OR, AM_R_R, RT_A, RT_B},
                [0xB1] = {IN_OR, AM_R_R, RT_A, RT_C},
                [0xB2] = {IN_OR, AM_R_R, RT_A, RT_D},
                [0xB3] = {IN_OR, AM_R_R, RT_A, RT_E},
                [0xB4] = {IN_OR, AM_R_R, RT_A, RT_H},
                [0xB5] = {IN_OR, AM_R_R, RT_A, RT_L},
                [0xB6] = {IN_OR, AM_R_MR, RT_A, RT_HL},
                [0xB7] = {IN_OR, AM_R_R, RT_A, RT_A},
                [0xB8] = {IN_CP, AM_R_R, RT_A, RT_B},
                [0xB9] = {IN_CP, AM_R_R, RT_A, RT_C},
                [0xBA] = {IN_CP, AM_R_R, RT_A, RT_D},
                [0xBB] = {IN_CP, AM_R_R, RT_A, RT_E},
                [0xBC] = {IN_CP, AM_R_R, RT_A, RT_H},
                [0xBD] = {IN_CP, AM_R_R, RT_A, RT_L},
                [0xBE] = {IN_CP, AM_R_MR, RT_A, RT_HL},
                [0xBF] = {IN_CP, AM_R_R, RT_A, RT_A},
                [0xC0] = {IN_RET, AM_IMP, RT_NONE, RT_NONE, CT_NZ},
                [0xC1] = {IN_POP, AM_R, RT_BC},
                [0xC2] = {IN_JP, AM_D16, RT_NONE, RT_NONE, CT_NZ},
                [0xC3] = {IN_JP, AM_D16},
                [0xC4] = {IN_CALL, AM_D16, RT_NONE, RT_NONE, CT_NZ},
                [0xC5] = {IN_PUSH, AM_R, RT_BC},
                [0xC6] = {IN_ADD, AM_R_D8, RT_A},
                [0xC7] = {IN_RST, AM_IMP, RT_NONE, RT_NONE, CT_NONE, 0x00},
                [0xC8] = {IN_RET, AM_IMP, RT_NONE, RT_NONE, CT_Z},
                [0xC9] = {IN_RET},
                [0xCA] = {IN_JP, AM_D16, RT_NONE, RT_NONE, CT_Z},
                [0xCB] = {IN_CB, AM_D8},
                [0xCC] = {IN_CALL, AM_D16, RT_NONE, RT_NONE, CT_Z},
                [0xCD] = {IN_CALL, AM_D16},
                [0xCE] = {IN_ADC, AM_R_D8, RT_A},
                [0xCF] = {IN_RST, AM_IMP, RT_NONE, RT_NONE, CT_NONE, 0x08},
                [0xD0] = {IN_RET, AM_IMP, RT_NONE, RT_NONE, CT_NC},
                [0xD1] = {IN_POP, AM_R, RT_DE},
                [0xD2] = {IN_JP, AM_D16, RT_NONE, RT_NONE, CT_NC},
                [0xD4] = {IN_CALL, AM_D16, RT_NONE, RT_NONE, CT_NC},
                [0xD5] = {IN_PUSH, AM_R, RT_DE},
                [0xD6] = {IN_SUB, AM_R_D8, RT_A},
                [0xD7] = {IN_RST, AM_IMP, RT_NONE, RT_NONE, CT_NONE, 0x10},
                [0xD8] = {IN_RET, AM_IMP, RT_NONE, RT_NONE, CT_C},
                [0xD9] = {IN_RETI},
                [0xDA] = {IN_JP, AM_D16, RT_NONE, RT_NONE, CT_C},
                [0xDC] = {IN_CALL, AM_D16, RT_NONE, RT_NONE, CT_C},
                [0xDE] = {IN_SBC, AM_R_D8, RT_A},
                [0xDF] = {IN_RST, AM_IMP, RT_NONE, RT_NONE, CT_NONE, 0x18},
                [0xE0] = {IN_LDH, AM_A8_R, RT_NONE, RT_A},
                [0xE1] = {IN_POP, AM_R, RT_HL},
                [0xE2] = {IN_LD, AM_MR_R, RT_C, RT_A},
                [0xE5] = {IN_PUSH, AM_R, RT_HL},
                [0xE6] = {IN_AND, AM_R_D8, RT_A},
                [0xE7] = {IN_RST, AM_IMP, RT_NONE, RT_NONE, CT_NONE, 0x20},
                [0xE8] = {IN_ADD, AM_R_D8, RT_SP},
                [0xE9] = {IN_JP, AM_R, RT_HL},
                [0xEA] = {IN_LD, AM_A16_R, RT_NONE, RT_A},
                [0xEE] = {IN_XOR, AM_R_D8, RT_A},
                [0xEF] = {IN_RST, AM_IMP, RT_NONE, RT_NONE, CT_NONE, 0x28},
                [0xF0] = {IN_LDH, AM_R_A8, RT_A},
                [0xF1] = {IN_POP, AM_R, RT_AF},
                [0xF2] = {IN_LD, AM_R_MR, RT_A, RT_C},
                [0xF3] = {IN_DI},
                [0xF5] = {IN_PUSH, AM_R, RT_AF},
                [0xF6] = {IN_OR, AM_R_D8, RT_A},
                [0xF7] = {IN_RST, AM_IMP, RT_NONE, RT_NONE, CT_NONE, 0x30},
                [0xF8] = {IN_LD, AM_HL_SPR, RT_HL, RT_SP},
                [0xF9] = {IN_LD, AM_R_R, RT_SP, RT_HL},
                [0xFA] = {IN_LD, AM_R_A16, RT_A},
                [0xFB] = {IN_EI},
                [0xFE] = {IN_CP, AM_R_D8, RT_A},
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
                [IN_INC] = proc_inc,
                [IN_DEC] = proc_dec,
                [IN_ADD] = proc_add,
                [IN_ADC] = proc_adc,
                [IN_SUB] = proc_sub,
                [IN_SBC] = proc_sbc,
                [IN_RETI] = proc_reti,
                [IN_XOR] = proc_xor,
                [IN_AND] = proc_and,
                [IN_OR] = proc_or,
                [IN_CP] = proc_cp,
                [IN_RRCA] = proc_rrca,
                [IN_RLCA] = proc_rlca,
                [IN_RRA] = proc_rra,
                [IN_RLA] = proc_rla,
                [IN_STOP] = proc_stop,
                [IN_CB] = proc_cb,
                [IN_HALT] = proc_halt,
                [IN_DAA] = proc_daa,
                [IN_CPL] = proc_cpl,
                [IN_SCF] = proc_scf,
                [IN_CCF] = proc_ccf,
                [IN_EI] = proc_ei,
            },
        .by_opcode = by_opcode,
        .lookup = lookup,
        .get_proc = get_proc,
};

const class_t *Instructions = (const class_t *) &init_instructions;
