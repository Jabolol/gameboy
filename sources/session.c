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
    const char *save = va_arg(*args, const char *);

    self->gameboy = NULL;

    GameboyClass *gameboy = new_class(Gameboy);
    if (!gameboy) {
        return;
    }

    if (!gameboy->boot(gameboy, rom, save)) {
        destroy_class(gameboy);
        return;
    }

    if (pthread_create(&self->thread, NULL, gameboy->cpu_run, gameboy) != 0) {
        destroy_class(gameboy);
        return;
    }

    self->set(self, gameboy);
}

static void destructor(void *ptr)
{
    SessionClass *self = (SessionClass *) ptr;
    GameboyClass *gameboy = self->get(self);
    if (gameboy) {
        atomic_store(&gameboy->context->die, true);
        pthread_join(self->thread, NULL);
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
