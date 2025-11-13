#include "../include/session.h"

static GameboyClass *get(SessionClass *self)
{
    return self->gameboy;
}

static void set(SessionClass *self, GameboyClass *gameboy)
{
    self->gameboy = gameboy;
}

static void constructor(void *ptr, va_list *args)
{
    SessionClass *self = (SessionClass *) ptr;
    const char *rom = va_arg(*args, const char *);

    GameboyClass *gameboy = new_class(Gameboy);
    if (!gameboy) {
        return;
    }

    if (!gameboy->cartridge->load(gameboy->cartridge, rom)) {
        destroy_class(gameboy);
        return;
    }

    bool cgb = gameboy->cartridge->context->header->cgb_flag & 0x80;
    gameboy->context->hw_mode = cgb ? HW_CGB : HW_DMG;
    gameboy->cpu->context->registers.a = cgb ? 0x11 : 0x01;
    gameboy->context->prev_frame = 0;

    if (pthread_create(&self->thread, NULL, gameboy->cpu_run, gameboy) != 0) {
        destroy_class(gameboy);
        return;
    }

    self->set(self, gameboy);
    LOG(cgb ? "Running in CGB mode" : "Running in DMG mode");
}

static void destructor(void *ptr)
{
    SessionClass *self = (SessionClass *) ptr;
    GameboyClass *gameboy = self->get(self);
    if (gameboy) {
        gameboy->context->die = true;
        destroy_class(gameboy);
    }
}

const SessionClass init_session = {
    {
        ._size = sizeof(SessionClass),
        ._name = "Session",
        ._constructor = constructor,
        ._destructor = destructor,
    },
    .get = get,
    .set = set,
};

const class_t *Session = (const class_t *) &init_session;
