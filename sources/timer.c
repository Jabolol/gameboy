#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    TimerClass *self = (TimerClass *) ptr;
    if (!((self->context = calloc(1, sizeof(*self->context))))) {
        HANDLE_ERROR("failed memory allocation");
    }
    self->parent = va_arg(*args, GameboyClass *);
    self->context->div = 0xABCC;
    self->context->tac = 0xF8;
}

static void destructor(void *ptr)
{
    TimerClass *self = (TimerClass *) ptr;
    free(self->context);
}

static bool timer_input(TimerClass *self, uint16_t div, uint8_t tac)
{
    return (tac & 0x04) && (div & self->tac_bits[tac & 0x03]);
}

static void increment_tima(TimerClass *self)
{
    self->context->tima += 1;
    if (self->context->tima == 0) {
        self->context->state = TIMA_OVERFLOW;
    }
}

static void set_div(TimerClass *self, uint16_t value)
{
    timer_context_t *ctx = self->context;
    uint16_t falling = ctx->div & ~value;
    uint16_t apu_bit = self->parent->context->double_speed ? 0x2000 : 0x1000;

    ctx->div = value;
    if ((ctx->tac & 0x04) && (falling & self->tac_bits[ctx->tac & 0x03])) {
        self->increment_tima(self);
    }
    if (falling & apu_bit) {
        self->parent->sound->sequencer_step(self->parent->sound);
    }
}

static void tick(TimerClass *self)
{
    timer_context_t *ctx = self->context;

    if (ctx->state == TIMA_RELOADING) {
        ctx->state = TIMA_COUNTING;
    } else if (ctx->state == TIMA_OVERFLOW) {
        ctx->state = TIMA_RELOADING;
        ctx->tima = ctx->tma;
        self->parent->cpu->request_interrupt(self->parent->cpu, IT_TIMER);
    }
    self->set_div(self, ctx->div + 4);
}

static void write(TimerClass *self, uint16_t address, uint8_t value)
{
    switch (address) {
        case DIV: self->set_div(self, 0); break;
        case TIMA: {
            if (self->context->state != TIMA_RELOADING) {
                self->context->state = TIMA_COUNTING;
                self->context->tima = value;
            }
            break;
        }
        case TMA: {
            self->context->tma = value;
            if (self->context->state == TIMA_RELOADING) {
                self->context->tima = value;
            }
            break;
        }
        case TAC: {
            uint8_t old = self->context->tac;
            self->context->tac = value | 0xF8;
            if (self->timer_input(self, self->context->div, old)
                && !self->timer_input(
                    self, self->context->div, self->context->tac)) {
                self->increment_tima(self);
            }
            break;
        }
        default: break;
    }
}

static uint8_t read(TimerClass *self, uint16_t address)
{
    switch (address) {
        case DIV: return self->context->div >> 8;
        case TIMA: return self->context->tima;
        case TMA: return self->context->tma;
        case TAC: return self->context->tac | 0xF8;
        default: return 0xFF;
    }
}

static void serialize(TimerClass *self, SnapshotClass *snapshot)
{
    snapshot->field(snapshot, self->context, sizeof(*self->context));
}

const TimerClass init_timer = {
    {
        ._size = sizeof(TimerClass),
        ._name = "Timer",
        ._constructor = constructor,
        ._destructor = destructor,
    },
    .tac_bits = {1 << 9, 1 << 3, 1 << 5, 1 << 7},
    .tick = tick,
    .write = write,
    .read = read,
    .set_div = set_div,
    .timer_input = timer_input,
    .increment_tima = increment_tima,
    .serialize = serialize,
};

const class_t *Timer = (const class_t *) &init_timer;
