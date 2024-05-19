#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    DMAClass *self = (DMAClass *) ptr;
    if (!((self->context = calloc(1, sizeof(*self->context))))) {
        HANDLE_ERROR("failed memory allocation");
    }
    self->parent = va_arg(*args, GameboyClass *);
}

static void destructor(void *ptr)
{
    DMAClass *self = (DMAClass *) ptr;
    free(self->context);
}

static void start(DMAClass *self, uint8_t value)
{
    self->context->byte = 0;
    self->context->active = true;
    self->context->start_delay = 2;
    self->context->value = value;
}

static void tick(DMAClass *self)
{
    if (!self->context->active) {
        return;
    }
    if (self->context->start_delay) {
        self->context->start_delay -= 1;
        return;
    }
    self->parent->ppu->oam_write(self->parent->ppu, self->context->byte,
        self->parent->bus->read(self->parent->bus,
            (self->context->value * 0x100) + self->context->byte));

    self->context->byte += 1;

    self->context->active = self->context->byte < 0xA0;
}

static bool transferring(DMAClass *self)
{
    return self->context->active;
}

const DMAClass init_dma = {
    {
        ._size = sizeof(DMAClass),
        ._name = "DMA",
        ._constructor = constructor,
        ._destructor = destructor,
    },
    .start = start,
    .tick = tick,
    .transferring = transferring,
};

const class_t *DMA = (const class_t *) &init_dma;
