#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    CPUClass *self = (CPUClass *) ptr;
    if (!((self->context = calloc(1, sizeof(*self->context))))) {
        HANDLE_ERROR("failed memory allocation");
    }
    self->bus = va_arg(*args, BusClass *);
    self->instructions = va_arg(*args, InstructionsClass *);
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
        case AM_R:
            self->context->fetched_data =
                self->read_register(self, self->context->inst->register_1);
            break;
        case AM_R_D8:
            self->context->fetched_data =
                self->bus->read(self->bus, self->context->registers.pc++);
            // TODO(jabolo): implement cycles
            break;
        case AM_D16: {
            uint16_t lo =
                self->bus->read(self->bus, self->context->registers.pc++);
            // TODO(jabolo): implement cycles
            uint16_t hi =
                self->bus->read(self->bus, self->context->registers.pc++);
            // TODO(jabolo): implement cycles
            self->context->fetched_data = lo | (hi << 8);
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
            return reverse(*((uint16_t *) &self->context->registers.a));
        case RT_BC:
            return reverse(*((uint16_t *) &self->context->registers.b));
        case RT_DE:
            return reverse(*((uint16_t *) &self->context->registers.d));
        case RT_HL:
            return reverse(*((uint16_t *) &self->context->registers.h));
        case RT_PC: return self->context->registers.pc;
        case RT_SP: return self->context->registers.sp;
        default: return 0;
    }
}

static bool step(CPUClass *self)
{
    if (!self->context->halted) {
        uint16_t pc = self->context->registers.pc;
        self->fetch_instructions(self);
        self->fetch_data(self);
        printf("%04X: %-7s (%02X %02X %02X) A: %02X B: %02X C: %02X\n", pc,
            self->instructions->lookup(
                self->instructions, self->context->inst->type),
            self->context->opcode, self->bus->read(self->bus, pc + 1),
            self->bus->read(self->bus, pc + 2), self->context->registers.a,
            self->context->registers.b, self->context->registers.c);
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
};

const class_t *CPU = (const class_t *) &init_CPU;
