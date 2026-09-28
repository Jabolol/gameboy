#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    JoypadClass *self = (JoypadClass *) ptr;
    if (!((self->context = calloc(1, sizeof(*self->context))))) {
        HANDLE_ERROR("failed memory allocation");
    }
    self->parent = va_arg(*args, GameboyClass *);

    self->context->button_selected = false;
    self->context->direction_selected = false;
    self->context->previous_lines = 0x0F;
}

static void destructor(void *ptr)
{
    JoypadClass *self = (JoypadClass *) ptr;
    free(self->context);
}

static uint8_t lines(JoypadClass *self)
{
    uint8_t pressed =
        atomic_load_explicit(&self->context->buttons, memory_order_relaxed);
    uint8_t out = 0x0F;

    if (!self->context->button_selected) {
        out &= ~(pressed & 0x0F);
    }
    if (!self->context->direction_selected) {
        out &= ~(pressed >> 4);
    }
    return out;
}

static void set_button(JoypadClass *self, button_t button, bool pressed)
{
    if (pressed) {
        atomic_fetch_or(&self->context->buttons, button);
    } else {
        atomic_fetch_and(&self->context->buttons, (uint8_t) ~button);
    }
}

static bool pressed(JoypadClass *self, button_t button)
{
    return atomic_load_explicit(&self->context->buttons, memory_order_relaxed)
        & button;
}

static void update(JoypadClass *self)
{
    uint8_t current = self->lines(self);

    if (self->context->previous_lines & ~current & 0x0F) {
        self->parent->cpu->request_interrupt(self->parent->cpu, IT_JOYPAD);
    }
    self->context->previous_lines = current;
}

static void choose(JoypadClass *self, uint8_t value)
{
    self->context->button_selected = value & 0x20;
    self->context->direction_selected = value & 0x10;
    self->update(self);
}

static uint8_t output(JoypadClass *self)
{
    return 0xC0 | (self->context->button_selected ? 0x20 : 0x00)
        | (self->context->direction_selected ? 0x10 : 0x00)
        | self->lines(self);
}

static bool any_pressed(JoypadClass *self)
{
    return atomic_load_explicit(&self->context->buttons, memory_order_relaxed);
}

static void serialize(JoypadClass *self, SnapshotClass *snapshot)
{
    joypad_context_t *ctx = self->context;

    snapshot->field(
        snapshot, &ctx->button_selected, sizeof(ctx->button_selected));
    snapshot->field(
        snapshot, &ctx->direction_selected, sizeof(ctx->direction_selected));
    snapshot->field(
        snapshot, &ctx->previous_lines, sizeof(ctx->previous_lines));
}

const JoypadClass init_joypad = {
    {
        ._size = sizeof(JoypadClass),
        ._name = "Joypad",
        ._constructor = constructor,
        ._destructor = destructor,
    },
    .choose = choose,
    .output = output,
    .lines = lines,
    .set_button = set_button,
    .pressed = pressed,
    .any_pressed = any_pressed,
    .update = update,
    .serialize = serialize,
};

const class_t *Joypad = (const class_t *) &init_joypad;
