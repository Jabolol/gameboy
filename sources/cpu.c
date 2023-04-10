#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    CPUClass *self = (CPUClass *) ptr;
    if (!((self->context = calloc(1, sizeof(*self->context))))) {
        HANDLE_ERROR("failed memory allocation");
    }
    self->bus = va_arg(*args, BusClass *);
    self->instructions = va_arg(*args, InstructionsClass *);
    self->parent = va_arg(*args, GameboyClass *);
}

static void destructor(void *ptr)
{
    CPUClass *self = (CPUClass *) ptr;
    free(self->context);
}

static void init(CPUClass *self)
{
    self->context->registers.pc = 0x100;
    self->context->registers.a = 0x01;
}

static void fetch_instructions(CPUClass *self)
{
    self->context->opcode =
        self->bus->read(self->bus, self->context->registers.pc++);
    self->context->inst = self->instructions->by_opcode(
        self->instructions, self->context->opcode);
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
        }
        case AM_R_D8: {
            self->context->fetched_data =
                self->bus->read(self->bus, self->context->registers.pc++);
            self->parent->cycles(self->parent, 1);
            break;
        }
        case AM_R_D16:
        case AM_D16: {
            uint16_t lo =
                self->bus->read(self->bus, self->context->registers.pc++);
            self->parent->cycles(self->parent, 1);
            uint16_t hi =
                self->bus->read(self->bus, self->context->registers.pc++);
            self->parent->cycles(self->parent, 1);
            self->context->fetched_data = lo | (hi << 8);
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
            self->context->fetched_data = self->bus->read(self->bus, address);
            self->parent->cycles(self->parent, 1);
            break;
        }
        case AM_R_HLI: {
            uint16_t address =
                self->read_register(self, self->context->inst->register_2);
            self->context->fetched_data = self->bus->read(self->bus, address);
            self->parent->cycles(self->parent, 1);
            self->set_register(
                self, RT_HL, self->read_register(self, RT_HL) + 1);
            break;
        }
        case AM_R_HLD: {
            uint16_t address =
                self->read_register(self, self->context->inst->register_2);
            self->context->fetched_data = self->bus->read(self->bus, address);
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
            self->context->fetched_data =
                self->bus->read(self->bus, self->context->registers.pc++);
            self->parent->cycles(self->parent, 1);
            break;
        }
        case AM_A8_R: {
            self->context->mem_dest =
                self->bus->read(self->bus, self->context->registers.pc++)
                | 0xFF00;
            self->context->dest_is_mem = true;
            self->parent->cycles(self->parent, 1);
            break;
        }
        case AM_HL_SPR: {
            self->context->fetched_data =
                self->bus->read(self->bus, self->context->registers.pc++);
            self->parent->cycles(self->parent, 1);
            break;
        }
        case AM_D8: {
            self->context->fetched_data =
                self->bus->read(self->bus, self->context->registers.pc++);
            self->parent->cycles(self->parent, 1);
            break;
        }
        case AM_A16_R:
        case AM_D16_R: {
            uint16_t lo =
                self->bus->read(self->bus, self->context->registers.pc++);
            self->parent->cycles(self->parent, 1);
            uint16_t hi =
                self->bus->read(self->bus, self->context->registers.pc++);
            self->parent->cycles(self->parent, 1);
            self->context->dest_is_mem = true;
            self->context->mem_dest = lo | (hi << 8);
            self->context->fetched_data =
                self->read_register(self, self->context->inst->register_2);
            break;
        }
        case AM_MR_D8: {
            self->context->fetched_data =
                self->bus->read(self->bus, self->context->registers.pc++);
            self->parent->cycles(self->parent, 1);
            self->context->dest_is_mem = true;
            self->context->mem_dest =
                self->read_register(self, self->context->inst->register_1);
            break;
        }
        case AM_MR: {
            uint16_t address =
                self->read_register(self, self->context->inst->register_1);
            self->context->fetched_data = self->bus->read(self->bus, address);
            self->context->dest_is_mem = true;
            self->context->mem_dest =
                self->read_register(self, self->context->inst->register_1);
            self->parent->cycles(self->parent, 1);
            break;
        }
        case AM_R_A16: {
            uint16_t lo =
                self->bus->read(self->bus, self->context->registers.pc++);
            self->parent->cycles(self->parent, 1);
            uint16_t hi =
                self->bus->read(self->bus, self->context->registers.pc++);
            self->parent->cycles(self->parent, 1);
            uint16_t address = lo | (hi << 8);
            self->context->fetched_data = self->bus->read(self->bus, address);
            self->parent->cycles(self->parent, 1);
            break;
        }
        default: {
            char buff[256];
            sprintf(buff, "Unknown Addressing Mode! %d (%02X)\n",
                self->context->inst->mode, self->context->opcode);
            HANDLE_ERROR(buff);
        }
    }
}

static void execute(CPUClass *self)
{
    proc_fn proc = self->instructions->get_proc(
        self->instructions, self->context->inst->type);

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

static bool step(CPUClass *self)
{
    if (!self->context->halted) {
        uint16_t pc = self->context->registers.pc;
        self->fetch_instructions(self);
        self->fetch_data(self);
        printf("%04X: %-7s (%02X %02X %02X) A: %02X BC: %02X%02X DE: %02X%02X "
               "HL: %02X%02X\n",
            pc,
            self->instructions->lookup(
                self->instructions, self->context->inst->type),
            self->context->opcode, self->bus->read(self->bus, pc + 1),
            self->bus->read(self->bus, pc + 2), self->context->registers.a,
            self->context->registers.b, self->context->registers.c,
            self->context->registers.d, self->context->registers.e,
            self->context->registers.h, self->context->registers.l);

        if (self->context->inst == NULL) {
            char buff[256];
            sprintf(
                buff, "Unknown Instruction! %02X\n", self->context->opcode);
            HANDLE_ERROR(buff);
        }
        self->execute(self);
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

const CPUClass init_CPU = {
    {
        ._size = sizeof(CPUClass),
        ._name = "CPU",
        ._constructor = constructor,
        ._destructor = destructor,
    },
    .bus = NULL,
    .instructions = NULL,
    .init = init,
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
};

const class_t *CPU = (const class_t *) &init_CPU;
