#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    CPUClass *self = (CPUClass *) ptr;
    if (!((self->context = calloc(1, sizeof(*self->context))))) {
        HANDLE_ERROR("failed memory allocation");
    }
    self->parent = va_arg(*args, GameboyClass *);
    self->context->registers.pc = 0x100;
    self->context->registers.sp = 0xFFFE;
    *((int16_t *) &self->context->registers.a) = 0xB001;
    *((int16_t *) &self->context->registers.b) = 0x1300;
    *((int16_t *) &self->context->registers.d) = 0xD800;
    *((int16_t *) &self->context->registers.h) = 0x4D01;
    self->context->ie_register = 0;
    self->context->int_flags = 0;
    self->context->int_master_enabled = false;
    self->context->enabling_ime = false;
    self->parent->timer->context->div = 0xABCC;
}

static void destructor(void *ptr)
{
    CPUClass *self = (CPUClass *) ptr;
    free(self->context);
}

static void fetch_instructions(CPUClass *self)
{
    self->context->opcode = self->parent->bus->read(
        self->parent->bus, self->get_registers(self)->pc++);
    self->context->inst = self->parent->instructions->by_opcode(
        self->parent->instructions, self->context->opcode);
}

static void fetch_data(CPUClass *self)
{
    self->context->mem_dest = 0;
    self->context->dest_is_mem = false;

    if (self->context->inst == NULL) {
        return;
    }
    switch (self->context->inst->mode) {
        case AM_IMP: break;
        case AM_R: {
            self->context->fetched_data =
                self->read_register(self, self->context->inst->register_1);
            break;
        }
        case AM_R_R: {
            self->context->fetched_data =
                self->read_register(self, self->context->inst->register_2);
            break;
        }
        case AM_R_D8: {
            self->context->fetched_data = self->parent->bus->read(
                self->parent->bus, self->context->registers.pc);
            self->parent->cycles(self->parent, 1);
            self->context->registers.pc++;
            break;
        }
        case AM_R_D16:
        case AM_D16: {
            uint16_t lo = self->parent->bus->read(
                self->parent->bus, self->context->registers.pc);
            self->parent->cycles(self->parent, 1);
            uint16_t hi = self->parent->bus->read(
                self->parent->bus, self->context->registers.pc + 1);
            self->parent->cycles(self->parent, 1);
            self->context->fetched_data = lo | (hi << 8);
            self->context->registers.pc += 2;
            break;
        }
        case AM_MR_R: {
            self->context->fetched_data =
                self->read_register(self, self->context->inst->register_2);
            self->context->mem_dest =
                self->read_register(self, self->context->inst->register_1);
            self->context->dest_is_mem = true;
            if (self->context->inst->register_1 == RT_C) {
                self->context->mem_dest |= 0xFF00;
            }
            break;
        }
        case AM_R_MR: {
            uint16_t address =
                self->read_register(self, self->context->inst->register_2);
            if (self->context->inst->register_2 == RT_C) {
                address |= 0xFF00;
            }
            self->context->fetched_data =
                self->parent->bus->read(self->parent->bus, address);
            self->parent->cycles(self->parent, 1);
            break;
        }
        case AM_R_HLI: {
            uint16_t address =
                self->read_register(self, self->context->inst->register_2);
            self->context->fetched_data =
                self->parent->bus->read(self->parent->bus, address);
            self->parent->cycles(self->parent, 1);
            self->set_register(
                self, RT_HL, self->read_register(self, RT_HL) + 1);
            break;
        }
        case AM_R_HLD: {
            uint16_t address =
                self->read_register(self, self->context->inst->register_2);
            self->context->fetched_data =
                self->parent->bus->read(self->parent->bus, address);
            self->parent->cycles(self->parent, 1);
            self->set_register(
                self, RT_HL, self->read_register(self, RT_HL) - 1);
            break;
        }
        case AM_HLI_R: {
            self->context->fetched_data =
                self->read_register(self, self->context->inst->register_2);
            self->context->mem_dest =
                self->read_register(self, self->context->inst->register_1);
            self->context->dest_is_mem = true;
            self->set_register(
                self, RT_HL, self->read_register(self, RT_HL) + 1);
            break;
        }
        case AM_HLD_R: {
            self->context->fetched_data =
                self->read_register(self, self->context->inst->register_2);
            self->context->mem_dest =
                self->read_register(self, self->context->inst->register_1);
            self->context->dest_is_mem = true;
            self->set_register(
                self, RT_HL, self->read_register(self, RT_HL) - 1);
            break;
        }
        case AM_R_A8: {
            self->context->fetched_data = self->parent->bus->read(
                self->parent->bus, self->context->registers.pc);
            self->parent->cycles(self->parent, 1);
            self->context->registers.pc++;
            break;
        }
        case AM_A8_R: {
            self->context->mem_dest =
                self->parent->bus->read(
                    self->parent->bus, self->context->registers.pc)
                | 0xFF00;
            self->context->dest_is_mem = true;
            self->parent->cycles(self->parent, 1);
            self->context->registers.pc++;
            break;
        }
        case AM_HL_SPR: {
            self->context->fetched_data = self->parent->bus->read(
                self->parent->bus, self->context->registers.pc);
            self->parent->cycles(self->parent, 1);
            self->context->registers.pc++;
            break;
        }
        case AM_D8: {
            self->context->fetched_data = self->parent->bus->read(
                self->parent->bus, self->context->registers.pc);
            self->parent->cycles(self->parent, 1);
            self->context->registers.pc++;
            break;
        }
        case AM_A16_R:
        case AM_D16_R: {
            uint16_t lo = self->parent->bus->read(
                self->parent->bus, self->context->registers.pc);
            self->parent->cycles(self->parent, 1);
            uint16_t hi = self->parent->bus->read(
                self->parent->bus, self->context->registers.pc + 1);
            self->parent->cycles(self->parent, 1);
            self->context->dest_is_mem = true;
            self->context->mem_dest = lo | (hi << 8);
            self->context->fetched_data =
                self->read_register(self, self->context->inst->register_2);
            self->context->registers.pc += 2;
            break;
        }
        case AM_MR_D8: {
            self->context->fetched_data = self->parent->bus->read(
                self->parent->bus, self->context->registers.pc);
            self->parent->cycles(self->parent, 1);
            self->context->registers.pc++;
            self->context->dest_is_mem = true;
            self->context->mem_dest =
                self->read_register(self, self->context->inst->register_1);
            break;
        }
        case AM_MR: {
            uint16_t address =
                self->read_register(self, self->context->inst->register_1);
            self->context->fetched_data =
                self->parent->bus->read(self->parent->bus, address);
            self->context->dest_is_mem = true;
            self->context->mem_dest =
                self->read_register(self, self->context->inst->register_1);
            self->parent->cycles(self->parent, 1);
            break;
        }
        case AM_R_A16: {
            uint16_t lo = self->parent->bus->read(
                self->parent->bus, self->context->registers.pc);
            self->parent->cycles(self->parent, 1);
            uint16_t hi = self->parent->bus->read(
                self->parent->bus, self->context->registers.pc + 1);
            self->parent->cycles(self->parent, 1);
            uint16_t address = lo | (hi << 8);
            self->context->fetched_data =
                self->parent->bus->read(self->parent->bus, address);
            self->context->registers.pc += 2;
            self->parent->cycles(self->parent, 1);
            break;
        }
        default: {
            char buff[64];
            snprintf(buff, sizeof(buff),
                "Unknown Addressing Mode! %d (%02X)\n",
                self->context->inst->mode, self->context->opcode);
            HANDLE_ERROR(buff);
        }
    }
}

static void execute(CPUClass *self)
{
    proc_fn proc = self->parent->instructions->get_proc(
        self->parent->instructions, self->context->inst->type);

    if (!proc) {
        HANDLE_ERROR("not implemented");
    }
    proc(self);
}

static uint16_t reverse(uint16_t n)
{
    return ((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8);
}

static void set_flags(CPUClass *self, char z, char n, char h, char c)
{
    if (z != -1) {
        BIT_SET(self->context->registers.f, 7, z);
    }
    if (n != -1) {
        BIT_SET(self->context->registers.f, 6, n);
    }
    if (h != -1) {
        BIT_SET(self->context->registers.f, 5, h);
    }
    if (c != -1) {
        BIT_SET(self->context->registers.f, 4, c);
    }
}

static bool check_condition(CPUClass *self)
{
    bool z = BIT(self->context->registers.f, 7);
    bool c = BIT(self->context->registers.f, 4);

    switch (self->context->inst->condition) {
        case CT_NONE: return true;
        case CT_C: return c;
        case CT_NC: return !c;
        case CT_Z: return z;
        case CT_NZ: return !z;
    }

    return false;
}

static uint16_t read_register(CPUClass *self, register_type_t type)
{
    switch (type) {
        case RT_A: return self->context->registers.a;
        case RT_F: return self->context->registers.f;
        case RT_B: return self->context->registers.b;
        case RT_C: return self->context->registers.c;
        case RT_D: return self->context->registers.d;
        case RT_E: return self->context->registers.e;
        case RT_H: return self->context->registers.h;
        case RT_L: return self->context->registers.l;
        case RT_AF:
            return self->reverse(*((uint16_t *) &self->context->registers.a));
        case RT_BC:
            return self->reverse(*((uint16_t *) &self->context->registers.b));
        case RT_DE:
            return self->reverse(*((uint16_t *) &self->context->registers.d));
        case RT_HL:
            return self->reverse(*((uint16_t *) &self->context->registers.h));
        case RT_PC: return self->context->registers.pc;
        case RT_SP: return self->context->registers.sp;
        default: return 0;
    }
}

static void set_register(CPUClass *self, register_type_t type, uint16_t val)
{
    switch (type) {
        case RT_A: self->context->registers.a = val & 0xFF; break;
        case RT_F: self->context->registers.f = val & 0xFF; break;
        case RT_B: self->context->registers.b = val & 0xFF; break;
        case RT_C: self->context->registers.c = val & 0xFF; break;
        case RT_D: self->context->registers.d = val & 0xFF; break;
        case RT_E: self->context->registers.e = val & 0xFF; break;
        case RT_H: self->context->registers.h = val & 0xFF; break;
        case RT_L: self->context->registers.l = val & 0xFF; break;

        case RT_AF:
            *((uint16_t *) &self->context->registers.a) = self->reverse(val);
            break;
        case RT_BC:
            *((uint16_t *) &self->context->registers.b) = self->reverse(val);
            break;
        case RT_DE:
            *((uint16_t *) &self->context->registers.d) = self->reverse(val);
            break;
        case RT_HL:
            *((uint16_t *) &self->context->registers.h) = self->reverse(val);
            break;

        case RT_PC: self->context->registers.pc = val; break;
        case RT_SP: self->context->registers.sp = val; break;
        case RT_NONE: break;
    }
}

static uint8_t read_register8(CPUClass *self, register_type_t reg)
{
    switch (reg) {
        case RT_A: return self->context->registers.a;
        case RT_F: return self->context->registers.f;
        case RT_B: return self->context->registers.b;
        case RT_C: return self->context->registers.c;
        case RT_D: return self->context->registers.d;
        case RT_E: return self->context->registers.e;
        case RT_H: return self->context->registers.h;
        case RT_L: return self->context->registers.l;
        case RT_HL:
            return self->parent->bus->read(
                self->parent->bus, self->read_register(self, RT_HL));
        default: HANDLE_ERROR("Invalid register read");
    }
}

static void set_register8(CPUClass *self, register_type_t reg, uint8_t val)
{
    switch (reg) {
        case RT_A: self->context->registers.a = val & 0xFF; break;
        case RT_F: self->context->registers.f = val & 0xFF; break;
        case RT_B: self->context->registers.b = val & 0xFF; break;
        case RT_C: self->context->registers.c = val & 0xFF; break;
        case RT_D: self->context->registers.d = val & 0xFF; break;
        case RT_E: self->context->registers.e = val & 0xFF; break;
        case RT_H: self->context->registers.h = val & 0xFF; break;
        case RT_L: self->context->registers.l = val & 0xFF; break;
        case RT_HL:
            self->parent->bus->write(
                self->parent->bus, self->read_register(self, RT_HL), val);
            break;
        default: HANDLE_ERROR("Invalid register set");
    }
}

static bool step(CPUClass *self)
{
    if (!self->context->halted) {
        uint16_t pc = self->context->registers.pc;
        self->fetch_instructions(self);
        self->parent->cycles(self->parent, 1);
        self->fetch_data(self);

        char flags[16];
        snprintf(flags, sizeof(flags), "%c%c%c%c",
            self->context->registers.f & (1 << 7) ? 'Z' : '-',
            self->context->registers.f & (1 << 6) ? 'N' : '-',
            self->context->registers.f & (1 << 5) ? 'H' : '-',
            self->context->registers.f & (1 << 4) ? 'C' : '-');

        char instruction_data[INST_BUFF_LEN];
        self->pretty_instruction(self, instruction_data);

        printf(
            "%08llX - %04X: %-12s (%02X %02X %02X) A: %02X F: %s BC: %02X%02X "
            "DE: "
            "%02X%02X "
            "HL: %02X%02X\n",
            self->parent->context->ticks, pc, instruction_data,
            self->context->opcode,
            self->parent->bus->read(self->parent->bus, pc + 1),
            self->parent->bus->read(self->parent->bus, pc + 2),
            self->context->registers.a, flags, self->context->registers.b,
            self->context->registers.c, self->context->registers.d,
            self->context->registers.e, self->context->registers.h,
            self->context->registers.l);

        if (self->context->inst == NULL) {
            char buff[64];
            snprintf(buff, sizeof(buff), "Unknown Instruction! %02X\n",
                self->context->opcode);
            HANDLE_ERROR(buff);
        }
        self->parent->debug->update(self->parent->debug);
        self->parent->debug->print(self->parent->debug);
        self->execute(self);
    } else {
        self->parent->cycles(self->parent, 1);
        if (self->context->int_flags) {
            self->context->halted = false;
        }
    }
    if (self->context->int_master_enabled) {
        self->handle_interrupts(self);
        self->context->enabling_ime = false;
    }
    if (self->context->enabling_ime) {
        self->context->int_master_enabled = true;
    }
    return true;
}

static void set_ie_register(CPUClass *self, uint8_t value)
{
    self->context->ie_register = value;
}

static uint8_t get_ie_register(CPUClass *self)
{
    return self->context->ie_register;
}

static registers_t *get_registers(CPUClass *self)
{
    return &self->context->registers;
}

static bool is_16bit(register_type_t reg)
{
    return reg >= RT_AF;
}

static register_type_t decode_register(CPUClass *self, uint8_t reg)
{
    if (reg > 0b111) {
        return RT_NONE;
    }

    return self->register_lookup[reg];
}

static uint8_t get_int_flags(CPUClass *self)
{
    return self->context->int_flags;
}

static void set_int_flags(CPUClass *self, uint8_t value)
{
    self->context->int_flags = value;
}

static void int_handle(CPUClass *self, uint16_t address)
{
    self->parent->stack->push16(
        self->parent->stack, self->context->registers.pc);
    self->context->registers.pc = address;
}

static bool int_check(CPUClass *self, uint16_t address, interrupt_t interr)
{
    if (self->context->int_flags & interr
        && self->context->ie_register & interr) {
        self->int_handle(self, address);
        self->context->int_flags &= ~interr;
        self->context->halted = false;
        self->context->int_master_enabled = false;
        return true;
    }
    return false;
}

static void handle_interrupts(CPUClass *self)
{
    self->int_check(self, 0x40, IT_VBLANK);
    self->int_check(self, 0x48, IT_LCD_STAT);
    self->int_check(self, 0x50, IT_TIMER);
    self->int_check(self, 0x58, IT_SERIAL);
    self->int_check(self, 0x60, IT_JOYPAD);
}

static void request_interrupt(CPUClass *self, interrupt_t type)
{
    self->context->int_flags |= type;
}

static void pretty_instruction(CPUClass *self, char buff[INST_BUFF_LEN])
{
    instruction_t *instruction = self->context->inst;
    char *instruction_name = self->parent->instructions->lookup(
        self->parent->instructions, instruction->type);

    switch (instruction->mode) {
        case AM_IMP:
            snprintf(buff, INST_BUFF_LEN, "%s ", instruction_name);
            break;
        case AM_R_D16:
        case AM_R_A16:
            snprintf(buff, INST_BUFF_LEN, "%s %s,$%04X", instruction_name,
                LOOKUP_REG1, self->context->fetched_data);
            break;
        case AM_R:
            snprintf(
                buff, INST_BUFF_LEN, "%s %s", instruction_name, LOOKUP_REG1);
            break;
        case AM_R_R:
            snprintf(buff, INST_BUFF_LEN, "%s %s,%s", instruction_name,
                LOOKUP_REG1, LOOKUP_REG2);
            break;
        case AM_MR_R:
            snprintf(buff, INST_BUFF_LEN, "%s (%s),%s", instruction_name,
                LOOKUP_REG1, LOOKUP_REG2);
            break;
        case AM_MR:
            snprintf(
                buff, INST_BUFF_LEN, "%s (%s)", instruction_name, LOOKUP_REG1);
            break;
        case AM_R_MR:
            snprintf(buff, INST_BUFF_LEN, "%s %s,(%s)", instruction_name,
                LOOKUP_REG1, LOOKUP_REG2);
            break;
        case AM_R_D8:
        case AM_R_A8:
            snprintf(buff, INST_BUFF_LEN, "%s %s,$%02X", instruction_name,
                LOOKUP_REG1, self->context->fetched_data & 0xFF);
            break;
        case AM_R_HLI:
            snprintf(buff, INST_BUFF_LEN, "%s %s,(%s+)", instruction_name,
                LOOKUP_REG1, LOOKUP_REG2);
            break;
        case AM_R_HLD:
            snprintf(buff, INST_BUFF_LEN, "%s %s,(%s-)", instruction_name,
                LOOKUP_REG1, LOOKUP_REG2);
            break;
        case AM_HLI_R:
            snprintf(buff, INST_BUFF_LEN, "%s (%s+),%s", instruction_name,
                LOOKUP_REG1, LOOKUP_REG2);
            break;
        case AM_HLD_R:
            snprintf(buff, INST_BUFF_LEN, "%s (%s-),%s", instruction_name,
                LOOKUP_REG1, LOOKUP_REG2);
            break;
        case AM_A8_R:
            snprintf(buff, INST_BUFF_LEN, "%s $%02X,%s", instruction_name,
                self->parent->bus->read(
                    self->parent->bus, self->context->registers.pc - 1),
                LOOKUP_REG2);
            break;
        case AM_HL_SPR:
            snprintf(buff, INST_BUFF_LEN, "%s (%s),SP+%d", instruction_name,
                LOOKUP_REG1, self->context->fetched_data & 0xFF);
            break;
        case AM_D8:
            snprintf(buff, INST_BUFF_LEN, "%s $%02X", instruction_name,
                self->context->fetched_data & 0xFF);
            break;
        case AM_D16:
            snprintf(buff, INST_BUFF_LEN, "%s $%04X", instruction_name,
                self->context->fetched_data);
            break;
        case AM_MR_D8:
            snprintf(buff, INST_BUFF_LEN, "%s (%s),$%02X", instruction_name,
                LOOKUP_REG1, self->context->fetched_data & 0xFF);
            break;
        case AM_A16_R:
            snprintf(buff, INST_BUFF_LEN, "%s ($%04X),%s", instruction_name,
                self->context->fetched_data, LOOKUP_REG2);
            break;
        default: {
            HANDLE_ERROR("Invalid Addressing Mode");
        }
    }
}

const CPUClass init_CPU = {
    {
        ._size = sizeof(CPUClass),
        ._name = "CPU",
        ._constructor = constructor,
        ._destructor = destructor,
    },
    .register_lookup =
        {
            RT_B,
            RT_C,
            RT_D,
            RT_E,
            RT_H,
            RT_L,
            RT_HL,
            RT_A,
        },
    .str_register_lookup =
        {
            "<NONE>",
            "A",
            "F",
            "B",
            "C",
            "D",
            "E",
            "H",
            "L",
            "AF",
            "BC",
            "DE",
            "HL",
            "SP",
            "PC",
        },
    .step = step,
    .set_flags = set_flags,
    .fetch_instructions = fetch_instructions,
    .fetch_data = fetch_data,
    .execute = execute,
    .reverse = reverse,
    .read_register = read_register,
    .check_condition = check_condition,
    .set_register = set_register,
    .set_ie_register = set_ie_register,
    .get_ie_register = get_ie_register,
    .get_registers = get_registers,
    .is_16bit = is_16bit,
    .read_register8 = read_register8,
    .set_register8 = set_register8,
    .decode_register = decode_register,
    .int_handle = int_handle,
    .int_check = int_check,
    .set_int_flags = set_int_flags,
    .get_int_flags = get_int_flags,
    .request_interrupt = request_interrupt,
    .handle_interrupts = handle_interrupts,
    .pretty_instruction = pretty_instruction,
};

const class_t *CPU = (const class_t *) &init_CPU;
