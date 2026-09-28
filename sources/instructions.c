#include "../include/gameboy.h"

static instruction_t *by_opcode(InstructionsClass *self, uint8_t opcode)
{
    return self->instructions + opcode;
}

static char *lookup(InstructionsClass *self, instruction_type_t instruction)
{
    return self->lookup_table[instruction];
}

static void proc_none(CPUClass *cpu)
{
    cpu->lock(cpu);
}

static void proc_nop(CPUClass UNUSED *cpu)
{
    return;
}

static void proc_di(CPUClass *cpu)
{
    cpu->context->int_master_enabled = false;
    cpu->context->enabling_ime = false;
}

static void proc_ei(CPUClass *cpu)
{
    cpu->context->enabling_ime = true;
}

static void proc_ld(CPUClass *cpu)
{
    instruction_t *inst = cpu->context->inst;

    if (cpu->context->dest_is_mem) {
        if (cpu->is_16bit(inst->register_2)) {
            cpu->write_cycle(cpu, cpu->context->mem_dest,
                cpu->context->fetched_data & 0xFF);
            cpu->write_cycle(cpu, cpu->context->mem_dest + 1,
                cpu->context->fetched_data >> 8);
        } else {
            cpu->write_cycle(
                cpu, cpu->context->mem_dest, cpu->context->fetched_data);
        }
        return;
    }
    if (inst->mode == AM_HL_SPR) {
        uint16_t sp = cpu->context->registers.sp;
        uint8_t offset = cpu->context->fetched_data & 0xFF;

        cpu->set_flags(cpu, 0, 0, (sp & 0xF) + (offset & 0xF) > 0xF,
            (sp & 0xFF) + offset > 0xFF);
        cpu->set_register(cpu, RT_HL, sp + (int8_t) offset);
        cpu->parent->cycles(cpu->parent, 1);
        return;
    }
    if (inst->register_1 == RT_SP && inst->register_2 == RT_HL) {
        cpu->parent->cycles(cpu->parent, 1);
    }
    cpu->set_register(cpu, inst->register_1, cpu->context->fetched_data);
}

static void proc_ldh(CPUClass *cpu)
{
    if (cpu->context->inst->register_1 == RT_A) {
        cpu->context->registers.a =
            cpu->read_cycle(cpu, 0xFF00 | cpu->context->fetched_data);
    } else {
        cpu->write_cycle(
            cpu, cpu->context->mem_dest, cpu->context->registers.a);
    }
}

static void proc_rlca(CPUClass *cpu)
{
    uint8_t u = cpu->context->registers.a;
    bool c = (u >> 7) & 1;

    cpu->context->registers.a = (u << 1) | c;
    cpu->set_flags(cpu, 0, 0, 0, c);
}

static void proc_rrca(CPUClass *cpu)
{
    uint8_t b = cpu->context->registers.a & 1;

    cpu->context->registers.a = (cpu->context->registers.a >> 1) | (b << 7);
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

static void proc_rra(CPUClass *cpu)
{
    uint8_t carry = CPU_FLAG_C;
    uint8_t new_c = cpu->context->registers.a & 1;

    cpu->context->registers.a =
        (cpu->context->registers.a >> 1) | (carry << 7);
    cpu->set_flags(cpu, 0, 0, 0, new_c);
}

static void proc_stop(CPUClass *cpu)
{
    GameboyClass *gameboy = cpu->parent;

    cpu->context->registers.pc++;
    gameboy->timer->write(gameboy->timer, DIV, 0);

    if (gameboy->context->hw_mode == HW_CGB
        && gameboy->context->speed_switch_armed) {
        gameboy->context->speed_switch_armed = false;
        gameboy->context->double_speed = !gameboy->context->double_speed;
        gameboy->cycles(gameboy, 2050);
        return;
    }
    cpu->context->stopped = true;
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
    if (!cpu->context->int_master_enabled && cpu->pending_interrupts(cpu)) {
        cpu->context->halt_bug = true;
        return;
    }
    cpu->context->halted = true;
}

static void proc_and(CPUClass *cpu)
{
    cpu->context->registers.a &= cpu->context->fetched_data & 0xFF;
    cpu->set_flags(cpu, cpu->context->registers.a == 0, 0, 1, 0);
}

static void proc_or(CPUClass *cpu)
{
    cpu->context->registers.a |= cpu->context->fetched_data & 0xFF;
    cpu->set_flags(cpu, cpu->context->registers.a == 0, 0, 0, 0);
}

static void proc_xor(CPUClass *cpu)
{
    cpu->context->registers.a ^= cpu->context->fetched_data & 0xFF;
    cpu->set_flags(cpu, cpu->context->registers.a == 0, 0, 0, 0);
}

static void proc_cp(CPUClass *cpu)
{
    uint8_t a = cpu->context->registers.a;
    uint8_t value = cpu->context->fetched_data & 0xFF;

    cpu->set_flags(cpu, a == value, 1, (a & 0x0F) < (value & 0x0F), a < value);
}

static void proc_cb(CPUClass *cpu)
{
    uint8_t op = cpu->context->fetched_data;
    register_type_t reg = cpu->decode_register(cpu, op & 0b111);
    uint8_t bit = (op >> 3) & 0b111;
    uint8_t bit_op = (op >> 6) & 0b11;
    uint8_t reg_val = cpu->read_register8(cpu, reg);

    switch (bit_op) {
        case CB_BIT: {
            cpu->set_flags(cpu, !(reg_val & (1 << bit)), 0, 1, -1);
            return;
        }
        case CB_RST: {
            cpu->set_register8(cpu, reg, reg_val & ~(1 << bit));
            return;
        }
        case CB_SET: {
            cpu->set_register8(cpu, reg, reg_val | (1 << bit));
            return;
        }
    }

    bool flag_c = CPU_FLAG_C;
    uint8_t result = 0;
    bool carry = false;

    switch (bit) {
        case CB_RLC: {
            result = (reg_val << 1) | (reg_val >> 7);
            carry = reg_val & 0x80;
            break;
        }
        case CB_RRC: {
            result = (reg_val >> 1) | (reg_val << 7);
            carry = reg_val & 1;
            break;
        }
        case CB_RL: {
            result = (reg_val << 1) | flag_c;
            carry = reg_val & 0x80;
            break;
        }
        case CB_RR: {
            result = (reg_val >> 1) | (flag_c << 7);
            carry = reg_val & 1;
            break;
        }
        case CB_SLA: {
            result = reg_val << 1;
            carry = reg_val & 0x80;
            break;
        }
        case CB_SRA: {
            result = (reg_val >> 1) | (reg_val & 0x80);
            carry = reg_val & 1;
            break;
        }
        case CB_SWP: {
            result = ((reg_val & 0xF0) >> 4) | ((reg_val & 0x0F) << 4);
            carry = false;
            break;
        }
        case CB_SRL: {
            result = reg_val >> 1;
            carry = reg_val & 1;
            break;
        }
    }
    cpu->set_register8(cpu, reg, result);
    cpu->set_flags(cpu, result == 0, 0, 0, carry);
}

static void proc_jp(CPUClass *cpu)
{
    if (!cpu->check_condition(cpu)) {
        return;
    }
    cpu->context->registers.pc = cpu->context->fetched_data;
    if (cpu->context->inst->mode != AM_R) {
        cpu->parent->cycles(cpu->parent, 1);
    }
}

static void proc_jr(CPUClass *cpu)
{
    int8_t offset = (int8_t) (cpu->context->fetched_data & 0xFF);

    if (!cpu->check_condition(cpu)) {
        return;
    }
    cpu->context->registers.pc += offset;
    cpu->parent->cycles(cpu->parent, 1);
}

static void proc_call(CPUClass *cpu)
{
    if (!cpu->check_condition(cpu)) {
        return;
    }
    cpu->parent->cycles(cpu->parent, 1);
    cpu->push_cycle(cpu, cpu->context->registers.pc);
    cpu->context->registers.pc = cpu->context->fetched_data;
}

static void proc_rst(CPUClass *cpu)
{
    cpu->parent->cycles(cpu->parent, 1);
    cpu->push_cycle(cpu, cpu->context->registers.pc);
    cpu->context->registers.pc = cpu->context->inst->parameter;
}

static void proc_ret(CPUClass *cpu)
{
    if (cpu->context->inst->condition != CT_NONE) {
        cpu->parent->cycles(cpu->parent, 1);
    }
    if (!cpu->check_condition(cpu)) {
        return;
    }
    cpu->context->registers.pc = cpu->pop_cycle(cpu);
    cpu->parent->cycles(cpu->parent, 1);
}

static void proc_reti(CPUClass *cpu)
{
    cpu->context->int_master_enabled = true;
    cpu->context->enabling_ime = false;
    proc_ret(cpu);
}

static void proc_pop(CPUClass *cpu)
{
    cpu->set_register(
        cpu, cpu->context->inst->register_1, cpu->pop_cycle(cpu));
}

static void proc_push(CPUClass *cpu)
{
    cpu->parent->cycles(cpu->parent, 1);
    cpu->push_cycle(
        cpu, cpu->read_register(cpu, cpu->context->inst->register_1));
}

static void proc_inc(CPUClass *cpu)
{
    instruction_t *inst = cpu->context->inst;

    if (inst->mode == AM_MR) {
        uint8_t value = cpu->context->fetched_data + 1;
        cpu->write_cycle(cpu, cpu->context->mem_dest, value);
        cpu->set_flags(cpu, value == 0, 0, (value & 0x0F) == 0, -1);
        return;
    }
    if (cpu->is_16bit(inst->register_1)) {
        cpu->set_register(
            cpu, inst->register_1, cpu->context->fetched_data + 1);
        cpu->parent->cycles(cpu->parent, 1);
        return;
    }

    uint8_t value = cpu->context->fetched_data + 1;
    cpu->set_register(cpu, inst->register_1, value);
    cpu->set_flags(cpu, value == 0, 0, (value & 0x0F) == 0, -1);
}

static void proc_dec(CPUClass *cpu)
{
    instruction_t *inst = cpu->context->inst;

    if (inst->mode == AM_MR) {
        uint8_t value = cpu->context->fetched_data - 1;
        cpu->write_cycle(cpu, cpu->context->mem_dest, value);
        cpu->set_flags(cpu, value == 0, 1, (value & 0x0F) == 0x0F, -1);
        return;
    }
    if (cpu->is_16bit(inst->register_1)) {
        cpu->set_register(
            cpu, inst->register_1, cpu->context->fetched_data - 1);
        cpu->parent->cycles(cpu->parent, 1);
        return;
    }

    uint8_t value = cpu->context->fetched_data - 1;
    cpu->set_register(cpu, inst->register_1, value);
    cpu->set_flags(cpu, value == 0, 1, (value & 0x0F) == 0x0F, -1);
}

static void proc_add(CPUClass *cpu)
{
    instruction_t *inst = cpu->context->inst;

    if (inst->register_1 == RT_SP) {
        uint16_t sp = cpu->context->registers.sp;
        uint8_t offset = cpu->context->fetched_data & 0xFF;

        cpu->set_flags(cpu, 0, 0, (sp & 0xF) + (offset & 0xF) > 0xF,
            (sp & 0xFF) + offset > 0xFF);
        cpu->context->registers.sp = sp + (int8_t) offset;
        cpu->parent->cycles(cpu->parent, 2);
        return;
    }
    if (cpu->is_16bit(inst->register_1)) {
        uint16_t hl = cpu->read_register(cpu, RT_HL);
        uint16_t value = cpu->context->fetched_data;
        uint32_t result = (uint32_t) hl + value;

        cpu->set_flags(cpu, -1, 0, (hl & 0xFFF) + (value & 0xFFF) > 0xFFF,
            result > 0xFFFF);
        cpu->set_register(cpu, RT_HL, result & 0xFFFF);
        cpu->parent->cycles(cpu->parent, 1);
        return;
    }

    uint8_t a = cpu->context->registers.a;
    uint8_t value = cpu->context->fetched_data & 0xFF;
    uint16_t result = a + value;

    cpu->context->registers.a = result & 0xFF;
    cpu->set_flags(cpu, (result & 0xFF) == 0, 0,
        (a & 0xF) + (value & 0xF) > 0xF, result > 0xFF);
}

static void proc_adc(CPUClass *cpu)
{
    uint16_t u = cpu->context->fetched_data & 0xFF;
    uint16_t a = cpu->context->registers.a;
    uint16_t c = CPU_FLAG_C;

    cpu->context->registers.a = (a + u + c) & 0xFF;
    cpu->set_flags(cpu, cpu->context->registers.a == 0, 0,
        (a & 0xF) + (u & 0xF) + c > 0xF, a + u + c > 0xFF);
}

static void proc_sub(CPUClass *cpu)
{
    uint8_t a = cpu->context->registers.a;
    uint8_t value = cpu->context->fetched_data & 0xFF;

    cpu->context->registers.a = a - value;
    cpu->set_flags(cpu, a == value, 1, (a & 0x0F) < (value & 0x0F), a < value);
}

static void proc_sbc(CPUClass *cpu)
{
    int32_t a = cpu->context->registers.a;
    int32_t value = cpu->context->fetched_data & 0xFF;
    int32_t c = CPU_FLAG_C;
    int32_t result = a - value - c;

    cpu->context->registers.a = result & 0xFF;
    cpu->set_flags(cpu, (result & 0xFF) == 0, 1,
        (a & 0x0F) - (value & 0x0F) - c < 0, result < 0);
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
        .instructions =
            {
                [0x00] = {IN_NOP, AM_IMP, 0, 0, 0, 0},
                [0x01] = {IN_LD, AM_R_D16, RT_BC, 0, 0, 0},
                [0x02] = {IN_LD, AM_MR_R, RT_BC, RT_A, 0, 0},
                [0x03] = {IN_INC, AM_R, RT_BC, 0, 0, 0},
                [0x04] = {IN_INC, AM_R, RT_B, 0, 0, 0},
                [0x05] = {IN_DEC, AM_R, RT_B, 0, 0, 0},
                [0x06] = {IN_LD, AM_R_D8, RT_B, 0, 0, 0},
                [0x07] = {IN_RLCA, 0, 0, 0, 0, 0},
                [0x08] = {IN_LD, AM_A16_R, RT_NONE, RT_SP, 0, 0},
                [0x09] = {IN_ADD, AM_R_R, RT_HL, RT_BC, 0, 0},
                [0x0A] = {IN_LD, AM_R_MR, RT_A, RT_BC, 0, 0},
                [0x0B] = {IN_DEC, AM_R, RT_BC, 0, 0, 0},
                [0x0C] = {IN_INC, AM_R, RT_C, 0, 0, 0},
                [0x0D] = {IN_DEC, AM_R, RT_C, 0, 0, 0},
                [0x0E] = {IN_LD, AM_R_D8, RT_C, 0, 0, 0},
                [0x0F] = {IN_RRCA, 0, 0, 0, 0, 0},
                [0x10] = {IN_STOP, 0, 0, 0, 0, 0},
                [0x11] = {IN_LD, AM_R_D16, RT_DE, 0, 0, 0},
                [0x12] = {IN_LD, AM_MR_R, RT_DE, RT_A, 0, 0},
                [0x13] = {IN_INC, AM_R, RT_DE, 0, 0, 0},
                [0x14] = {IN_INC, AM_R, RT_D, 0, 0, 0},
                [0x15] = {IN_DEC, AM_R, RT_D, 0, 0, 0},
                [0x16] = {IN_LD, AM_R_D8, RT_D, 0, 0, 0},
                [0x17] = {IN_RLA, 0, 0, 0, 0, 0},
                [0x18] = {IN_JR, AM_D8, 0, 0, 0, 0},
                [0x19] = {IN_ADD, AM_R_R, RT_HL, RT_DE, 0, 0},
                [0x1A] = {IN_LD, AM_R_MR, RT_A, RT_DE, 0, 0},
                [0x1B] = {IN_DEC, AM_R, RT_DE, 0, 0, 0},
                [0x1C] = {IN_INC, AM_R, RT_E, 0, 0, 0},
                [0x1D] = {IN_DEC, AM_R, RT_E, 0, 0, 0},
                [0x1E] = {IN_LD, AM_R_D8, RT_E, 0, 0, 0},
                [0x1F] = {IN_RRA, 0, 0, 0, 0, 0},
                [0x20] = {IN_JR, AM_D8, RT_NONE, RT_NONE, CT_NZ, 0},
                [0x21] = {IN_LD, AM_R_D16, RT_HL, 0, 0, 0},
                [0x22] = {IN_LD, AM_HLI_R, RT_HL, RT_A, 0, 0},
                [0x23] = {IN_INC, AM_R, RT_HL, 0, 0, 0},
                [0x24] = {IN_INC, AM_R, RT_H, 0, 0, 0},
                [0x25] = {IN_DEC, AM_R, RT_H, 0, 0, 0},
                [0x26] = {IN_LD, AM_R_D8, RT_H, 0, 0, 0},
                [0x27] = {IN_DAA, 0, 0, 0, 0, 0},
                [0x28] = {IN_JR, AM_D8, RT_NONE, RT_NONE, CT_Z, 0},
                [0x29] = {IN_ADD, AM_R_R, RT_HL, RT_HL, 0, 0},
                [0x2A] = {IN_LD, AM_R_HLI, RT_A, RT_HL, 0, 0},
                [0x2B] = {IN_DEC, AM_R, RT_HL, 0, 0, 0},
                [0x2C] = {IN_INC, AM_R, RT_L, 0, 0, 0},
                [0x2D] = {IN_DEC, AM_R, RT_L, 0, 0, 0},
                [0x2E] = {IN_LD, AM_R_D8, RT_L, 0, 0, 0},
                [0x2F] = {IN_CPL, 0, 0, 0, 0, 0},
                [0x30] = {IN_JR, AM_D8, RT_NONE, RT_NONE, CT_NC, 0},
                [0x31] = {IN_LD, AM_R_D16, RT_SP, 0, 0, 0},
                [0x32] = {IN_LD, AM_HLD_R, RT_HL, RT_A, 0, 0},
                [0x33] = {IN_INC, AM_R, RT_SP, 0, 0, 0},
                [0x34] = {IN_INC, AM_MR, RT_HL, 0, 0, 0},
                [0x35] = {IN_DEC, AM_MR, RT_HL, 0, 0, 0},
                [0x36] = {IN_LD, AM_MR_D8, RT_HL, 0, 0, 0},
                [0x37] = {IN_SCF, 0, 0, 0, 0, 0},
                [0x38] = {IN_JR, AM_D8, RT_NONE, RT_NONE, CT_C, 0},
                [0x39] = {IN_ADD, AM_R_R, RT_HL, RT_SP, 0, 0},
                [0x3A] = {IN_LD, AM_R_HLD, RT_A, RT_HL, 0, 0},
                [0x3B] = {IN_DEC, AM_R, RT_SP, 0, 0, 0},
                [0x3C] = {IN_INC, AM_R, RT_A, 0, 0, 0},
                [0x3D] = {IN_DEC, AM_R, RT_A, 0, 0, 0},
                [0x3E] = {IN_LD, AM_R_D8, RT_A, 0, 0, 0},
                [0x3F] = {IN_CCF, 0, 0, 0, 0, 0},
                [0x40] = {IN_LD, AM_R_R, RT_B, RT_B, 0, 0},
                [0x41] = {IN_LD, AM_R_R, RT_B, RT_C, 0, 0},
                [0x42] = {IN_LD, AM_R_R, RT_B, RT_D, 0, 0},
                [0x43] = {IN_LD, AM_R_R, RT_B, RT_E, 0, 0},
                [0x44] = {IN_LD, AM_R_R, RT_B, RT_H, 0, 0},
                [0x45] = {IN_LD, AM_R_R, RT_B, RT_L, 0, 0},
                [0x46] = {IN_LD, AM_R_MR, RT_B, RT_HL, 0, 0},
                [0x47] = {IN_LD, AM_R_R, RT_B, RT_A, 0, 0},
                [0x48] = {IN_LD, AM_R_R, RT_C, RT_B, 0, 0},
                [0x49] = {IN_LD, AM_R_R, RT_C, RT_C, 0, 0},
                [0x4A] = {IN_LD, AM_R_R, RT_C, RT_D, 0, 0},
                [0x4B] = {IN_LD, AM_R_R, RT_C, RT_E, 0, 0},
                [0x4C] = {IN_LD, AM_R_R, RT_C, RT_H, 0, 0},
                [0x4D] = {IN_LD, AM_R_R, RT_C, RT_L, 0, 0},
                [0x4E] = {IN_LD, AM_R_MR, RT_C, RT_HL, 0, 0},
                [0x4F] = {IN_LD, AM_R_R, RT_C, RT_A, 0, 0},
                [0x50] = {IN_LD, AM_R_R, RT_D, RT_B, 0, 0},
                [0x51] = {IN_LD, AM_R_R, RT_D, RT_C, 0, 0},
                [0x52] = {IN_LD, AM_R_R, RT_D, RT_D, 0, 0},
                [0x53] = {IN_LD, AM_R_R, RT_D, RT_E, 0, 0},
                [0x54] = {IN_LD, AM_R_R, RT_D, RT_H, 0, 0},
                [0x55] = {IN_LD, AM_R_R, RT_D, RT_L, 0, 0},
                [0x56] = {IN_LD, AM_R_MR, RT_D, RT_HL, 0, 0},
                [0x57] = {IN_LD, AM_R_R, RT_D, RT_A, 0, 0},
                [0x58] = {IN_LD, AM_R_R, RT_E, RT_B, 0, 0},
                [0x59] = {IN_LD, AM_R_R, RT_E, RT_C, 0, 0},
                [0x5A] = {IN_LD, AM_R_R, RT_E, RT_D, 0, 0},
                [0x5B] = {IN_LD, AM_R_R, RT_E, RT_E, 0, 0},
                [0x5C] = {IN_LD, AM_R_R, RT_E, RT_H, 0, 0},
                [0x5D] = {IN_LD, AM_R_R, RT_E, RT_L, 0, 0},
                [0x5E] = {IN_LD, AM_R_MR, RT_E, RT_HL, 0, 0},
                [0x5F] = {IN_LD, AM_R_R, RT_E, RT_A, 0, 0},
                [0x60] = {IN_LD, AM_R_R, RT_H, RT_B, 0, 0},
                [0x61] = {IN_LD, AM_R_R, RT_H, RT_C, 0, 0},
                [0x62] = {IN_LD, AM_R_R, RT_H, RT_D, 0, 0},
                [0x63] = {IN_LD, AM_R_R, RT_H, RT_E, 0, 0},
                [0x64] = {IN_LD, AM_R_R, RT_H, RT_H, 0, 0},
                [0x65] = {IN_LD, AM_R_R, RT_H, RT_L, 0, 0},
                [0x66] = {IN_LD, AM_R_MR, RT_H, RT_HL, 0, 0},
                [0x67] = {IN_LD, AM_R_R, RT_H, RT_A, 0, 0},
                [0x68] = {IN_LD, AM_R_R, RT_L, RT_B, 0, 0},
                [0x69] = {IN_LD, AM_R_R, RT_L, RT_C, 0, 0},
                [0x6A] = {IN_LD, AM_R_R, RT_L, RT_D, 0, 0},
                [0x6B] = {IN_LD, AM_R_R, RT_L, RT_E, 0, 0},
                [0x6C] = {IN_LD, AM_R_R, RT_L, RT_H, 0, 0},
                [0x6D] = {IN_LD, AM_R_R, RT_L, RT_L, 0, 0},
                [0x6E] = {IN_LD, AM_R_MR, RT_L, RT_HL, 0, 0},
                [0x6F] = {IN_LD, AM_R_R, RT_L, RT_A, 0, 0},
                [0x70] = {IN_LD, AM_MR_R, RT_HL, RT_B, 0, 0},
                [0x71] = {IN_LD, AM_MR_R, RT_HL, RT_C, 0, 0},
                [0x72] = {IN_LD, AM_MR_R, RT_HL, RT_D, 0, 0},
                [0x73] = {IN_LD, AM_MR_R, RT_HL, RT_E, 0, 0},
                [0x74] = {IN_LD, AM_MR_R, RT_HL, RT_H, 0, 0},
                [0x75] = {IN_LD, AM_MR_R, RT_HL, RT_L, 0, 0},
                [0x76] = {IN_HALT, 0, 0, 0, 0, 0},
                [0x77] = {IN_LD, AM_MR_R, RT_HL, RT_A, 0, 0},
                [0x78] = {IN_LD, AM_R_R, RT_A, RT_B, 0, 0},
                [0x79] = {IN_LD, AM_R_R, RT_A, RT_C, 0, 0},
                [0x7A] = {IN_LD, AM_R_R, RT_A, RT_D, 0, 0},
                [0x7B] = {IN_LD, AM_R_R, RT_A, RT_E, 0, 0},
                [0x7C] = {IN_LD, AM_R_R, RT_A, RT_H, 0, 0},
                [0x7D] = {IN_LD, AM_R_R, RT_A, RT_L, 0, 0},
                [0x7E] = {IN_LD, AM_R_MR, RT_A, RT_HL, 0, 0},
                [0x7F] = {IN_LD, AM_R_R, RT_A, RT_A, 0, 0},
                [0x80] = {IN_ADD, AM_R_R, RT_A, RT_B, 0, 0},
                [0x81] = {IN_ADD, AM_R_R, RT_A, RT_C, 0, 0},
                [0x82] = {IN_ADD, AM_R_R, RT_A, RT_D, 0, 0},
                [0x83] = {IN_ADD, AM_R_R, RT_A, RT_E, 0, 0},
                [0x84] = {IN_ADD, AM_R_R, RT_A, RT_H, 0, 0},
                [0x85] = {IN_ADD, AM_R_R, RT_A, RT_L, 0, 0},
                [0x86] = {IN_ADD, AM_R_MR, RT_A, RT_HL, 0, 0},
                [0x87] = {IN_ADD, AM_R_R, RT_A, RT_A, 0, 0},
                [0x88] = {IN_ADC, AM_R_R, RT_A, RT_B, 0, 0},
                [0x89] = {IN_ADC, AM_R_R, RT_A, RT_C, 0, 0},
                [0x8A] = {IN_ADC, AM_R_R, RT_A, RT_D, 0, 0},
                [0x8B] = {IN_ADC, AM_R_R, RT_A, RT_E, 0, 0},
                [0x8C] = {IN_ADC, AM_R_R, RT_A, RT_H, 0, 0},
                [0x8D] = {IN_ADC, AM_R_R, RT_A, RT_L, 0, 0},
                [0x8E] = {IN_ADC, AM_R_MR, RT_A, RT_HL, 0, 0},
                [0x8F] = {IN_ADC, AM_R_R, RT_A, RT_A, 0, 0},
                [0x90] = {IN_SUB, AM_R_R, RT_A, RT_B, 0, 0},
                [0x91] = {IN_SUB, AM_R_R, RT_A, RT_C, 0, 0},
                [0x92] = {IN_SUB, AM_R_R, RT_A, RT_D, 0, 0},
                [0x93] = {IN_SUB, AM_R_R, RT_A, RT_E, 0, 0},
                [0x94] = {IN_SUB, AM_R_R, RT_A, RT_H, 0, 0},
                [0x95] = {IN_SUB, AM_R_R, RT_A, RT_L, 0, 0},
                [0x96] = {IN_SUB, AM_R_MR, RT_A, RT_HL, 0, 0},
                [0x97] = {IN_SUB, AM_R_R, RT_A, RT_A, 0, 0},
                [0x98] = {IN_SBC, AM_R_R, RT_A, RT_B, 0, 0},
                [0x99] = {IN_SBC, AM_R_R, RT_A, RT_C, 0, 0},
                [0x9A] = {IN_SBC, AM_R_R, RT_A, RT_D, 0, 0},
                [0x9B] = {IN_SBC, AM_R_R, RT_A, RT_E, 0, 0},
                [0x9C] = {IN_SBC, AM_R_R, RT_A, RT_H, 0, 0},
                [0x9D] = {IN_SBC, AM_R_R, RT_A, RT_L, 0, 0},
                [0x9E] = {IN_SBC, AM_R_MR, RT_A, RT_HL, 0, 0},
                [0x9F] = {IN_SBC, AM_R_R, RT_A, RT_A, 0, 0},
                [0xA0] = {IN_AND, AM_R_R, RT_A, RT_B, 0, 0},
                [0xA1] = {IN_AND, AM_R_R, RT_A, RT_C, 0, 0},
                [0xA2] = {IN_AND, AM_R_R, RT_A, RT_D, 0, 0},
                [0xA3] = {IN_AND, AM_R_R, RT_A, RT_E, 0, 0},
                [0xA4] = {IN_AND, AM_R_R, RT_A, RT_H, 0, 0},
                [0xA5] = {IN_AND, AM_R_R, RT_A, RT_L, 0, 0},
                [0xA6] = {IN_AND, AM_R_MR, RT_A, RT_HL, 0, 0},
                [0xA7] = {IN_AND, AM_R_R, RT_A, RT_A, 0, 0},
                [0xA8] = {IN_XOR, AM_R_R, RT_A, RT_B, 0, 0},
                [0xA9] = {IN_XOR, AM_R_R, RT_A, RT_C, 0, 0},
                [0xAA] = {IN_XOR, AM_R_R, RT_A, RT_D, 0, 0},
                [0xAB] = {IN_XOR, AM_R_R, RT_A, RT_E, 0, 0},
                [0xAC] = {IN_XOR, AM_R_R, RT_A, RT_H, 0, 0},
                [0xAD] = {IN_XOR, AM_R_R, RT_A, RT_L, 0, 0},
                [0xAE] = {IN_XOR, AM_R_MR, RT_A, RT_HL, 0, 0},
                [0xAF] = {IN_XOR, AM_R_R, RT_A, RT_A, 0, 0},
                [0xB0] = {IN_OR, AM_R_R, RT_A, RT_B, 0, 0},
                [0xB1] = {IN_OR, AM_R_R, RT_A, RT_C, 0, 0},
                [0xB2] = {IN_OR, AM_R_R, RT_A, RT_D, 0, 0},
                [0xB3] = {IN_OR, AM_R_R, RT_A, RT_E, 0, 0},
                [0xB4] = {IN_OR, AM_R_R, RT_A, RT_H, 0, 0},
                [0xB5] = {IN_OR, AM_R_R, RT_A, RT_L, 0, 0},
                [0xB6] = {IN_OR, AM_R_MR, RT_A, RT_HL, 0, 0},
                [0xB7] = {IN_OR, AM_R_R, RT_A, RT_A, 0, 0},
                [0xB8] = {IN_CP, AM_R_R, RT_A, RT_B, 0, 0},
                [0xB9] = {IN_CP, AM_R_R, RT_A, RT_C, 0, 0},
                [0xBA] = {IN_CP, AM_R_R, RT_A, RT_D, 0, 0},
                [0xBB] = {IN_CP, AM_R_R, RT_A, RT_E, 0, 0},
                [0xBC] = {IN_CP, AM_R_R, RT_A, RT_H, 0, 0},
                [0xBD] = {IN_CP, AM_R_R, RT_A, RT_L, 0, 0},
                [0xBE] = {IN_CP, AM_R_MR, RT_A, RT_HL, 0, 0},
                [0xBF] = {IN_CP, AM_R_R, RT_A, RT_A, 0, 0},
                [0xC0] = {IN_RET, AM_IMP, RT_NONE, RT_NONE, CT_NZ, 0},
                [0xC1] = {IN_POP, AM_R, RT_BC, 0, 0, 0},
                [0xC2] = {IN_JP, AM_D16, RT_NONE, RT_NONE, CT_NZ, 0},
                [0xC3] = {IN_JP, AM_D16, 0, 0, 0, 0},
                [0xC4] = {IN_CALL, AM_D16, RT_NONE, RT_NONE, CT_NZ, 0},
                [0xC5] = {IN_PUSH, AM_R, RT_BC, 0, 0, 0},
                [0xC6] = {IN_ADD, AM_R_D8, RT_A, 0, 0, 0},
                [0xC7] = {IN_RST, AM_IMP, RT_NONE, RT_NONE, CT_NONE, 0x00},
                [0xC8] = {IN_RET, AM_IMP, RT_NONE, RT_NONE, CT_Z, 0},
                [0xC9] = {IN_RET, 0, 0, 0, 0, 0},
                [0xCA] = {IN_JP, AM_D16, RT_NONE, RT_NONE, CT_Z, 0},
                [0xCB] = {IN_CB, AM_D8, 0, 0, 0, 0},
                [0xCC] = {IN_CALL, AM_D16, RT_NONE, RT_NONE, CT_Z, 0},
                [0xCD] = {IN_CALL, AM_D16, 0, 0, 0, 0},
                [0xCE] = {IN_ADC, AM_R_D8, RT_A, 0, 0, 0},
                [0xCF] = {IN_RST, AM_IMP, RT_NONE, RT_NONE, CT_NONE, 0x08},
                [0xD0] = {IN_RET, AM_IMP, RT_NONE, RT_NONE, CT_NC, 0},
                [0xD1] = {IN_POP, AM_R, RT_DE, 0, 0, 0},
                [0xD2] = {IN_JP, AM_D16, RT_NONE, RT_NONE, CT_NC, 0},
                [0xD4] = {IN_CALL, AM_D16, RT_NONE, RT_NONE, CT_NC, 0},
                [0xD5] = {IN_PUSH, AM_R, RT_DE, 0, 0, 0},
                [0xD6] = {IN_SUB, AM_R_D8, RT_A, 0, 0, 0},
                [0xD7] = {IN_RST, AM_IMP, RT_NONE, RT_NONE, CT_NONE, 0x10},
                [0xD8] = {IN_RET, AM_IMP, RT_NONE, RT_NONE, CT_C, 0},
                [0xD9] = {IN_RETI, 0, 0, 0, 0, 0},
                [0xDA] = {IN_JP, AM_D16, RT_NONE, RT_NONE, CT_C, 0},
                [0xDC] = {IN_CALL, AM_D16, RT_NONE, RT_NONE, CT_C, 0},
                [0xDE] = {IN_SBC, AM_R_D8, RT_A, 0, 0, 0},
                [0xDF] = {IN_RST, AM_IMP, RT_NONE, RT_NONE, CT_NONE, 0x18},
                [0xE0] = {IN_LDH, AM_A8_R, RT_NONE, RT_A, 0, 0},
                [0xE1] = {IN_POP, AM_R, RT_HL, 0, 0, 0},
                [0xE2] = {IN_LD, AM_MR_R, RT_C, RT_A, 0, 0},
                [0xE5] = {IN_PUSH, AM_R, RT_HL, 0, 0, 0},
                [0xE6] = {IN_AND, AM_R_D8, RT_A, 0, 0, 0},
                [0xE7] = {IN_RST, AM_IMP, RT_NONE, RT_NONE, CT_NONE, 0x20},
                [0xE8] = {IN_ADD, AM_R_D8, RT_SP, 0, 0, 0},
                [0xE9] = {IN_JP, AM_R, RT_HL, 0, 0, 0},
                [0xEA] = {IN_LD, AM_A16_R, RT_NONE, RT_A, 0, 0},
                [0xEE] = {IN_XOR, AM_R_D8, RT_A, 0, 0, 0},
                [0xEF] = {IN_RST, AM_IMP, RT_NONE, RT_NONE, CT_NONE, 0x28},
                [0xF0] = {IN_LDH, AM_R_A8, RT_A, 0, 0, 0},
                [0xF1] = {IN_POP, AM_R, RT_AF, 0, 0, 0},
                [0xF2] = {IN_LD, AM_R_MR, RT_A, RT_C, 0, 0},
                [0xF3] = {IN_DI, 0, 0, 0, 0, 0},
                [0xF5] = {IN_PUSH, AM_R, RT_AF, 0, 0, 0},
                [0xF6] = {IN_OR, AM_R_D8, RT_A, 0, 0, 0},
                [0xF7] = {IN_RST, AM_IMP, RT_NONE, RT_NONE, CT_NONE, 0x30},
                [0xF8] = {IN_LD, AM_HL_SPR, RT_HL, RT_SP, 0, 0},
                [0xF9] = {IN_LD, AM_R_R, RT_SP, RT_HL, 0, 0},
                [0xFA] = {IN_LD, AM_R_A16, RT_A, 0, 0, 0},
                [0xFB] = {IN_EI, 0, 0, 0, 0, 0},
                [0xFE] = {IN_CP, AM_R_D8, RT_A, 0, 0, 0},
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
