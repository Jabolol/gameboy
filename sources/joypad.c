#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    JoypadClass *self = (JoypadClass *) ptr;
    if (!((self->context = calloc(1, sizeof(*self->context))))) {
        HANDLE_ERROR("failed memory allocation");
    }
    self->parent = va_arg(*args, GameboyClass *);
}

static void destructor(void *ptr)
{
    JoypadClass *self = (JoypadClass *) ptr;
    free(self->context);
}

static void select(JoypadClass *self, uint8_t value)
{
    self->context->button_selected = value & 0x20;
    self->context->direction_selected = value & 0x10;
}

static uint8_t output(JoypadClass *self)
{
    uint8_t out = 0xCF;

    if (!self->context->button_selected) {
        if (self->context->state.start) {
            out &= ~(1 << 3);
        }
        if (self->context->state.select) {
            out &= ~(1 << 2);
        }
        if (self->context->state.a) {
            out &= ~(1 << 0);
        }
        if (self->context->state.b) {
            out &= ~(1 << 1);
        }
    }

    if (!self->context->direction_selected) {
        if (self->context->state.left) {
            out &= ~(1 << 1);
        }
        if (self->context->state.right) {
            out &= ~(1 << 0);
        }
        if (self->context->state.up) {
            out &= ~(1 << 2);
        }
        if (self->context->state.down) {
            out &= ~(1 << 3);
        }
    }

    return out;
}

const JoypadClass init_joypad = {
    {
        ._size = sizeof(JoypadClass),
        ._name = "Joypad",
        ._constructor = constructor,
        ._destructor = destructor,
    },
    .select = select,
    .output = output,
};

const class_t *Joypad = (const class_t *) &init_joypad;
