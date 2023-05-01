#include "SDL2/SDL.h"
#include "common.h"
#include "oop.h"
#include "SDL2/SDL_ttf.h"

#ifndef __UI
    #define __UI

typedef struct gameboy_aux GameboyClass;
typedef struct ui_aux UiClass;

typedef struct ui_aux {
    /* Properties */
    class_t metadata;
    GameboyClass *parent;
    int32_t screen_width;
    int32_t screen_height;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_Surface *screen;
    /* Methods */
    void (*handle_events)(UiClass *);
} UiClass;

extern const class_t *Ui;
#endif
