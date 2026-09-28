#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
    #include "../include/session.h"
#else
    #include "../include/gameboy.h"
#endif

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
SessionClass *gameboy_create(const char *rom, const char *save)
{
    SessionClass *self = new_class(Session, rom, save);
    return (self && self->get(self)) ? self : NULL;
}

EMSCRIPTEN_KEEPALIVE
void gameboy_start(SessionClass *self)
{
    if (self) {
        emscripten_set_main_loop_arg(
            self->get(self)->loop, self->get(self), 0, 1);
    }
}

EMSCRIPTEN_KEEPALIVE
void gameboy_destroy(SessionClass *self)
{
    if (self) {
        emscripten_cancel_main_loop();
        destroy_class(self);
    }
}

EMSCRIPTEN_KEEPALIVE
void gameboy_button(SessionClass *self, button_t button, bool pressed)
{
    JoypadClass *joypad = self->get(self)->joypad;
    joypad->set_button(joypad, button, pressed);
}

EMSCRIPTEN_KEEPALIVE
void gameboy_request(SessionClass *self, request_t request)
{
    self->get(self)->request(self->get(self), request);
}

EMSCRIPTEN_KEEPALIVE
void gameboy_configure(SessionClass *self, setting_t setting, uint32_t value)
{
    self->get(self)->configure(self->get(self), setting, value);
}

EMSCRIPTEN_KEEPALIVE
bool gameboy_rumble(SessionClass *self)
{
    return self->get(self)->cartridge->context->rumble;
}

EMSCRIPTEN_KEEPALIVE
bool gameboy_color(SessionClass *self)
{
    return self->get(self)->context->hw_mode == HW_CGB;
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
