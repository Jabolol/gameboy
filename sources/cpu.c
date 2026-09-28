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
    self->set_register(self, RT_AF, 0x01B0);
    self->set_register(self, RT_BC, 0x0013);
    self->set_register(self, RT_DE, 0x00D8);
    self->set_register(self, RT_HL, 0x014D);
    self->context->int_master_enabled = false;
    self->context->enabling_ime = false;
    self->parent->timer->context->div = 0xABCC;
}

static void destructor(void *ptr)
{
    CPUClass *self = (CPUClass *) ptr;
    free(self->context);
}

static uint8_t read_cycle(CPUClass *self, uint16_t address)
{
    uint8_t value = self->parent->bus->read(self->parent->bus, address);
    self->parent->cycles(self->parent, 1);
    return value;
}

static void write_cycle(CPUClass *self, uint16_t address, uint8_t value)
{
    self->parent->bus->write(self->parent->bus, address, value);
    self->parent->cycles(self->parent, 1);
}

static uint8_t fetch_byte(CPUClass *self)
{
    return self->read_cycle(self, self->context->registers.pc++);
}

static uint16_t fetch_word(CPUClass *self)
{
    uint16_t lo = self->fetch_byte(self);
    uint16_t hi = self->fetch_byte(self);
    return lo | (hi << 8);
}

static void push_cycle(CPUClass *self, uint16_t value)
{
    self->write_cycle(self, --self->context->registers.sp, value >> 8);
    self->write_cycle(self, --self->context->registers.sp, value & 0xFF);
}

static uint16_t pop_cycle(CPUClass *self)
{
    uint16_t lo = self->read_cycle(self, self->context->registers.sp++);
    uint16_t hi = self->read_cycle(self, self->context->registers.sp++);
    return lo | (hi << 8);
}

static void lock(CPUClass *self)
{
    char buff[64];
    snprintf(buff, sizeof(buff), "illegal opcode %02X at %04X, CPU locked",
        self->context->opcode, (uint16_t) (self->context->registers.pc - 1));
    WARN(buff);
    self->context->locked = true;
    self->context->int_master_enabled = false;
}

static void fetch_instructions(CPUClass *self)
{
    self->context->opcode =
        self->read_cycle(self, self->context->registers.pc);
    if (self->context->halt_bug) {
        self->context->halt_bug = false;
    } else {
        self->context->registers.pc++;
    }
    self->context->inst = self->parent->instructions->by_opcode(
        self->parent->instructions, self->context->opcode);
}

static void fetch_data(CPUClass *self)
{
    instruction_t *inst = self->context->inst;

    self->context->mem_dest = 0;
    self->context->dest_is_mem = false;

    switch (inst->mode) {
        case AM_IMP: break;
        case AM_R: {
            self->context->fetched_data =
                self->read_register(self, inst->register_1);
            break;
        }
        case AM_R_R: {
            self->context->fetched_data =
                self->read_register(self, inst->register_2);
            break;
        }
        case AM_R_D8:
        case AM_R_A8:
        case AM_HL_SPR:
        case AM_D8: {
            self->context->fetched_data = self->fetch_byte(self);
            break;
        }
        case AM_R_D16:
        case AM_D16: {
            self->context->fetched_data = self->fetch_word(self);
            break;
        }
        case AM_MR_R: {
            self->context->fetched_data =
                self->read_register(self, inst->register_2);
            self->context->mem_dest =
                self->read_register(self, inst->register_1);
            self->context->dest_is_mem = true;
            if (inst->register_1 == RT_C) {
                self->context->mem_dest |= 0xFF00;
            }
            break;
        }
        case AM_R_MR: {
            uint16_t address = self->read_register(self, inst->register_2);
            if (inst->register_2 == RT_C) {
                address |= 0xFF00;
            }
            self->context->fetched_data = self->read_cycle(self, address);
            break;
        }
        case AM_R_HLI:
        case AM_R_HLD: {
            uint16_t address = self->read_register(self, RT_HL);
            self->context->fetched_data = self->read_cycle(self, address);
            self->set_register(
                self, RT_HL, address + (inst->mode == AM_R_HLI ? 1 : -1));
            break;
        }
        case AM_HLI_R:
        case AM_HLD_R: {
            uint16_t address = self->read_register(self, RT_HL);
            self->context->fetched_data =
                self->read_register(self, inst->register_2);
            self->context->mem_dest = address;
            self->context->dest_is_mem = true;
            self->set_register(
                self, RT_HL, address + (inst->mode == AM_HLI_R ? 1 : -1));
            break;
        }
        case AM_A8_R: {
            self->context->mem_dest = self->fetch_byte(self) | 0xFF00;
            self->context->dest_is_mem = true;
            break;
        }
        case AM_A16_R:
        case AM_D16_R: {
            self->context->mem_dest = self->fetch_word(self);
            self->context->dest_is_mem = true;
            self->context->fetched_data =
                self->read_register(self, inst->register_2);
            break;
        }
        case AM_MR_D8: {
            self->context->fetched_data = self->fetch_byte(self);
            self->context->dest_is_mem = true;
            self->context->mem_dest =
                self->read_register(self, inst->register_1);
            break;
        }
        case AM_MR: {
            self->context->mem_dest =
                self->read_register(self, inst->register_1);
            self->context->dest_is_mem = true;
            self->context->fetched_data =
                self->read_cycle(self, self->context->mem_dest);
            break;
        }
        case AM_R_A16: {
            uint16_t address = self->fetch_word(self);
            self->context->fetched_data = self->read_cycle(self, address);
            break;
        }
    }
}

static void execute(CPUClass *self)
{
    proc_fn proc = self->parent->instructions->get_proc(
        self->parent->instructions, self->context->inst->type);

    proc(self);
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
    registers_t *r = &self->context->registers;

    switch (type) {
        case RT_A: return r->a;
        case RT_F: return r->f;
        case RT_B: return r->b;
        case RT_C: return r->c;
        case RT_D: return r->d;
        case RT_E: return r->e;
        case RT_H: return r->h;
        case RT_L: return r->l;
        case RT_AF: return (r->a << 8) | r->f;
        case RT_BC: return (r->b << 8) | r->c;
        case RT_DE: return (r->d << 8) | r->e;
        case RT_HL: return (r->h << 8) | r->l;
        case RT_PC: return r->pc;
        case RT_SP: return r->sp;
        default: return 0;
    }
}

static void set_register(CPUClass *self, register_type_t type, uint16_t val)
{
    registers_t *r = &self->context->registers;

    switch (type) {
        case RT_A: r->a = val & 0xFF; break;
        case RT_F: r->f = val & 0xF0; break;
        case RT_B: r->b = val & 0xFF; break;
        case RT_C: r->c = val & 0xFF; break;
        case RT_D: r->d = val & 0xFF; break;
        case RT_E: r->e = val & 0xFF; break;
        case RT_H: r->h = val & 0xFF; break;
        case RT_L: r->l = val & 0xFF; break;
        case RT_AF: {
            r->a = val >> 8;
            r->f = val & 0xF0;
            break;
        }
        case RT_BC: {
            r->b = val >> 8;
            r->c = val & 0xFF;
            break;
        }
        case RT_DE: {
            r->d = val >> 8;
            r->e = val & 0xFF;
            break;
        }
        case RT_HL: {
            r->h = val >> 8;
            r->l = val & 0xFF;
            break;
        }
        case RT_PC: r->pc = val; break;
        case RT_SP: r->sp = val; break;
        case RT_NONE: break;
    }
}

static uint8_t read_register8(CPUClass *self, register_type_t reg)
{
    if (reg == RT_HL) {
        return self->read_cycle(self, self->read_register(self, RT_HL));
    }
    return self->read_register(self, reg);
}

static void set_register8(CPUClass *self, register_type_t reg, uint8_t val)
{
    if (reg == RT_HL) {
        self->write_cycle(self, self->read_register(self, RT_HL), val);
        return;
    }
    self->set_register(self, reg, val);
}

static uint8_t pending_interrupts(CPUClass *self)
{
    return self->context->ie_register & self->context->int_flags & 0x1F;
}

static bool step(CPUClass *self)
{
    GameboyClass *gameboy = self->parent;

    if (self->context->locked) {
        gameboy->cycles(gameboy, 1);
        return true;
    }
    if (gameboy->lcd->context->hdma.pending && !self->context->halted) {
        gameboy->lcd->hdma_block(gameboy->lcd);
    }
    if (self->context->stopped) {
        gameboy->cycles(gameboy, 1);
        if (gameboy->joypad->any_pressed(gameboy->joypad)) {
            self->context->stopped = false;
        }
        return true;
    }
    if (self->context->halted) {
        gameboy->cycles(gameboy, 1);
        if (self->pending_interrupts(self)) {
            self->context->halted = false;
        }
    } else {
        bool enable_ime = self->context->enabling_ime;
#ifdef __CPU_DEBUG
        uint16_t pc = self->context->registers.pc;
#endif
        self->fetch_instructions(self);
        self->fetch_data(self);
#ifdef __CPU_DEBUG
        gameboy->debug->cpu_step(gameboy->debug, pc);
#endif
        self->execute(self);
        if (enable_ime && self->context->enabling_ime) {
            self->context->int_master_enabled = true;
            self->context->enabling_ime = false;
        }
    }
    if (self->context->int_master_enabled && self->pending_interrupts(self)) {
        self->handle_interrupts(self);
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

static void handle_interrupts(CPUClass *self)
{
    uint16_t pc = self->context->registers.pc;

    if (self->context->halt_bug) {
        self->context->halt_bug = false;
        pc -= 1;
    }

    self->context->int_master_enabled = false;
    self->context->enabling_ime = false;
    self->parent->cycles(self->parent, 2);
    self->write_cycle(self, --self->context->registers.sp, pc >> 8);

    uint8_t pending = self->pending_interrupts(self);
    uint16_t vector = 0x0000;

    self->write_cycle(self, --self->context->registers.sp, pc & 0xFF);

    for (uint8_t bit = 0; bit < 5; bit++) {
        if (pending & (1 << bit)) {
            vector = 0x40 + (bit * 8);
            self->context->int_flags &= ~(1 << bit);
            break;
        }
    }
    self->context->registers.pc = vector;
    self->parent->cycles(self->parent, 1);
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
                self->context->mem_dest & 0xFF, LOOKUP_REG2);
            break;
        case AM_HL_SPR:
            snprintf(buff, INST_BUFF_LEN, "%s (%s),SP+%d", instruction_name,
                LOOKUP_REG1, (int8_t) (self->context->fetched_data & 0xFF));
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
        case AM_D16_R:
            snprintf(buff, INST_BUFF_LEN, "%s ($%04X),%s", instruction_name,
                self->context->mem_dest, LOOKUP_REG2);
            break;
    }
}

static void serialize(CPUClass *self, SnapshotClass *snapshot)
{
    snapshot->field(snapshot, self->context, sizeof(*self->context));
    self->context->inst = self->parent->instructions->by_opcode(
        self->parent->instructions, self->context->opcode);
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
    .set_int_flags = set_int_flags,
    .get_int_flags = get_int_flags,
    .pending_interrupts = pending_interrupts,
    .request_interrupt = request_interrupt,
    .handle_interrupts = handle_interrupts,
    .read_cycle = read_cycle,
    .write_cycle = write_cycle,
    .fetch_byte = fetch_byte,
    .fetch_word = fetch_word,
    .push_cycle = push_cycle,
    .pop_cycle = pop_cycle,
    .lock = lock,
    .pretty_instruction = pretty_instruction,
    .serialize = serialize,
};

const class_t *CPU = (const class_t *) &init_CPU;
