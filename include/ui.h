#include <SDL2/SDL.h>
#include "common.h"
#include "oop.h"

#ifndef __UI
    #define __UI

typedef struct gameboy_aux GameboyClass;
typedef struct ui_aux UIClass;

typedef struct ui_aux {
    /* Properties */
    class_t metadata;
    GameboyClass *parent;
    int32_t scale;
    bool panel_visible;
    bool rumbling;
    uint32_t *panel_pixels;
    const uint32_t *tile_palettes[2][384];
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_Texture *panel_texture;
    SDL_GameController *controller;
    /* Methods */
    void (*create_resources)(UIClass *);
    int32_t (*panel_width)(UIClass *);
    void (*resize)(UIClass *);
    void (*handle_events)(UIClass *);
    void (*assign_palettes)(UIClass *);
    void (*display_tile)(UIClass *, uint8_t, uint16_t, int32_t, int32_t);
    void (*display_palettes)(UIClass *);
    void (*update_panel)(UIClass *);
    void (*update)(UIClass *);
    void (*update_rumble)(UIClass *);
    uint32_t (*get_ticks)(void);
    void (*delay)(uint32_t);
    void (*on_key)(UIClass *, bool, SDL_Keycode);
    void (*on_controller)(UIClass *, bool, uint8_t);
    void (*on_axis)(UIClass *, uint8_t, int16_t);
} UIClass;

extern const class_t *UI;
#endif
