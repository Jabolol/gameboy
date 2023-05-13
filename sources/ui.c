#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    UIClass *self = (UIClass *) ptr;
    self->parent = va_arg(*args, GameboyClass *);
    self->screen_height = va_arg(*args, int32_t);
    self->screen_width = va_arg(*args, int32_t);
    self->scale = va_arg(*args, int32_t);
    SDL_Init(SDL_INIT_VIDEO);
    LOG("SDL initialized\n");
    TTF_Init();
    LOG("TTF initialized\n");
    self->create_resources(self);
}

static void create_resources(UIClass *self)
{
    SDL_CreateWindowAndRenderer(self->screen_width, self->screen_height, 0,
        &self->window, &self->renderer);
    SDL_CreateWindowAndRenderer(16 * 8 * self->scale, 32 * 8 * self->scale, 0,
        &self->debug_window, &self->debug_renderer);
    self->debug_screen =
        SDL_CreateRGBSurface(0, (16 * 8 * self->scale) + (16 * self->scale),
            (32 * 8 * self->scale) + (64 * self->scale), 32, 0x00FF0000,
            0x0000FF00, 0x000000FF, 0xFF000000);
    self->debug_texture = SDL_CreateTexture(self->debug_renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
        (16 * 8 * self->scale) + (16 * self->scale),
        (32 * 8 * self->scale) + (64 * self->scale));
    SDL_GetWindowPosition(self->window, &self->x, &self->y);
    SDL_SetWindowPosition(
        self->debug_window, self->x + self->screen_width + 10, self->y);
}

static void destructor(void *ptr)
{
    UIClass *self = (UIClass *) ptr;
    SDL_DestroyWindow(self->window);
    SDL_DestroyRenderer(self->renderer);
    SDL_DestroyTexture(self->debug_texture);
    SDL_FreeSurface(self->debug_screen);
    SDL_DestroyWindow(self->debug_window);
    SDL_DestroyRenderer(self->debug_renderer);
    TTF_Quit();
    SDL_Quit();
}

static void handle_events(UIClass *self)
{
    SDL_Event event = {0};

    while (SDL_PollEvent(&event) > 0) {
        if (event.type == SDL_WINDOWEVENT
            && event.window.event == SDL_WINDOWEVENT_CLOSE) {
            self->parent->context->die = true;
        }
    }
}

static void update_debug_window(UIClass *self)
{
    int32_t x_draw = 0;
    int32_t y_draw = 0;
    int32_t tile_num = 0;
    int32_t width = self->debug_screen->w;
    int32_t height = self->debug_screen->h;

    SDL_FillRect(
        self->debug_screen, &(SDL_Rect){0, 0, width, height}, 0xFF111111);

    for (int32_t y = 0; y < 24; y++) {
        for (int32_t x = 0; x < 16; x++) {
            self->display_tile(self, tile_num, x_draw + (x * self->scale),
                y_draw + (y * self->scale));
            x_draw += (8 * self->scale);
            tile_num += 1;
        }
        y_draw += (8 * self->scale);
        x_draw = 0;
    }

    SDL_UpdateTexture(self->debug_texture, NULL, self->debug_screen->pixels,
        self->debug_screen->pitch);
    SDL_RenderClear(self->debug_renderer);
    SDL_RenderCopy(self->debug_renderer, self->debug_texture, NULL, NULL);
    SDL_RenderPresent(self->debug_renderer);
}

static void display_tile(
    UIClass *self, uint16_t tile_num, int32_t x, int32_t y)
{
    for (int32_t tile_y = 0; tile_y < 16; tile_y += 2) {
        uint8_t byte_1 = self->parent->bus->read(
            self->parent->bus, START_LOCATION + (tile_num * 16) + tile_y);
        uint8_t byte_2 = self->parent->bus->read(
            self->parent->bus, START_LOCATION + (tile_num * 16) + tile_y + 1);

        for (int32_t bit = 7; bit >= 0; bit--) {
            uint8_t hi = !!(byte_1 & (1 << bit)) << 1;
            uint8_t lo = !!(byte_2 & (1 << bit));
            uint8_t color = hi | lo;

            SDL_FillRect(self->debug_screen,
                &(SDL_Rect){
                    x + ((7 - bit) * self->scale),
                    y + (tile_y / 2 * self->scale),
                    self->scale,
                    self->scale,
                },
                self->tile_colors[color]);
        }
    }
}

static void update(UIClass *self)
{
    self->update_debug_window(self);
}

const UIClass init_ui = {
    {
        ._size = sizeof(UIClass),
        ._name = "UI",
        ._constructor = constructor,
        ._destructor = destructor,
    },
    .tile_colors = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000},
    .create_resources = create_resources,
    .handle_events = handle_events,
    .update_debug_window = update_debug_window,
    .display_tile = display_tile,
    .update = update,
};

const class_t *UI = (const class_t *) &init_ui;
