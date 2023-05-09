#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    UiClass *self = (UiClass *) ptr;
    self->parent = va_arg(*args, GameboyClass *);
    self->screen_height = va_arg(*args, int32_t);
    self->screen_width = va_arg(*args, int32_t);
    SDL_Init(SDL_INIT_VIDEO);
    printf("SDL initialized\n");
    TTF_Init();
    printf("TTF initialized\n");
    SDL_CreateWindowAndRenderer(self->screen_width, self->screen_height, 0,
        &self->window, &self->renderer);
}

static void destructor(void *ptr)
{
    UiClass *self = (UiClass *) ptr;
    SDL_DestroyWindow(self->window);
    SDL_DestroyRenderer(self->renderer);
    TTF_Quit();
    SDL_Quit();
}

static void handle_events(UiClass *self)
{
    SDL_Event event = {0};

    while (SDL_PollEvent(&event) > 0) {
        if (event.type == SDL_WINDOWEVENT
            && event.window.event == SDL_WINDOWEVENT_CLOSE) {
            self->parent->context->die = true;
        }
    }
}

const UiClass init_ui = {
    {
        ._size = sizeof(UiClass),
        ._name = "UI",
        ._constructor = constructor,
        ._destructor = destructor,
    },
    .handle_events = handle_events,
};

const class_t *Ui = (const class_t *) &init_ui;
