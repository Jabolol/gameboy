#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    TimerClass *self = (TimerClass *) ptr;
    if (!((self->context = calloc(1, sizeof(*self->context))))) {
        HANDLE_ERROR("failed memory allocation");
    }
    self->parent = va_arg(*args, GameboyClass *);
    self->context->div = 0xAC00;
}

static void destructor(void *ptr)
{
    CPUClass *self = (CPUClass *) ptr;
    free(self->context);
}

static void tick(TimerClass *self)
{
    bool timer_update = false;
    uint16_t prev_div = self->context->div;

    self->context->div += 1;
    switch (self->context->tac & (0b11)) {
        case 0b00:
            timer_update =
                (prev_div & (1 << 9)) && (!(self->context->div & (1 << 9)));
            break;
        case 0b01:
            timer_update =
                (prev_div & (1 << 3)) && (!(self->context->div & (1 << 3)));
            break;
        case 0b10:
            timer_update =
                (prev_div & (1 << 5)) && (!(self->context->div & (1 << 5)));
            break;
        case 0b11:
            timer_update =
                (prev_div & (1 << 7)) && (!(self->context->div & (1 << 7)));
            break;
    }
    if (timer_update && self->context->tac & (1 << 2)) {
        self->context->tima += 1;
        if (self->context->tima == 0xFF) {
            self->context->tima = self->context->tma;
            self->parent->cpu->request_interrupt(self->parent->cpu, IT_TIMER);
        }
    }
}

static void write(TimerClass *self, uint16_t address, uint8_t value)
{
    switch (address) {
        case DIV: self->context->div = 0; break;
        case TIMA: self->context->tima = value; break;
        case TMA: self->context->tma = value; break;
        case TAC: self->context->tac = value; break;
        default: {
            HANDLE_ERROR("Invalid Timer write");
        }
    }
}

static uint8_t read(TimerClass *self, uint16_t address)
{
    switch (address) {
        case DIV: return self->context->div >> 8;
        case TIMA: return self->context->tima;
        case TMA: return self->context->tma;
        case TAC: return self->context->tac;
        default: {
            HANDLE_ERROR("Invalid Timer read");
        }
    }
}

const TimerClass init_timer = {
    {
        ._size = sizeof(TimerClass),
        ._name = "Timer",
        ._constructor = constructor,
        ._destructor = destructor,
    },
    .tick = tick,
    .write = write,
    .read = read,
};

const class_t *Timer = (const class_t *) &init_timer;
