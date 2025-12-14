#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
    #include "../include/session.h"
#else
    #include "../include/gameboy.h"
#endif

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
SessionClass *gameboy_create(const char *rom)
{
    SessionClass *self = new_class(Session, rom);
    return (self && self->get(self)) ? self : NULL;
}

EMSCRIPTEN_KEEPALIVE
void gameboy_start(SessionClass *self)
{
    if (!self) {
        return;
    }
    GameboyClass *gameboy = self->get(self);
    if (!gameboy) {
        return;
    }
    emscripten_set_main_loop_arg(gameboy->loop, gameboy, 0, 1);
}

EMSCRIPTEN_KEEPALIVE
void gameboy_destroy(SessionClass *self)
{
    if (!self) {
        return;
    }
    destroy_class(self);
}
#endif

int main(int argc, char **argv)
{
#ifdef __EMSCRIPTEN__
    return 0;
#else
    GameboyClass *gameboy = new_class(Gameboy);
    if (!gameboy) {
        return 1;
    }
    int exit_code = gameboy->run(gameboy, argc, argv);
    destroy_class(gameboy);
    return exit_code;
#endif
}
