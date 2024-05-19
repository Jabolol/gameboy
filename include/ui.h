#include <SDL2/SDL.h>
#include "common.h"
#include "oop.h"
#include <SDL2/SDL_ttf.h>

#ifndef __UI
    #define __UI

typedef struct gameboy_aux GameboyClass;
typedef struct ui_aux UIClass;

typedef struct ui_aux {
    /* Properties */
    class_t metadata;
    GameboyClass *parent;
    int32_t screen_width;
    int32_t screen_height;
    int32_t scale;
    int32_t x;
    int32_t y;
    uint64_t tile_colors[4];
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_Surface *screen;
    SDL_Window *debug_window;
    SDL_Renderer *debug_renderer;
    SDL_Texture *debug_texture;
    SDL_Surface *debug_screen;
    /* Methods */
    void (*create_resources)(UIClass *);
    void (*handle_events)(UIClass *);
    void (*update_debug_window)(UIClass *);
    void (*display_tile)(UIClass *, uint16_t, int32_t, int32_t);
    void (*update)(UIClass *);
    uint32_t (*get_ticks)(void);
    void (*delay)(uint32_t);
    void (*on_key)(UIClass *, bool, SDL_Keycode);
} UIClass;

extern const class_t *UI;
#endif
