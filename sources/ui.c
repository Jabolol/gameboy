#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    UIClass *self = (UIClass *) ptr;
    self->parent = va_arg(*args, GameboyClass *);
    self->screen_height = va_arg(*args, int32_t);
    self->screen_width = va_arg(*args, int32_t);
    self->scale = va_arg(*args, int32_t);
    SDL_Init(SDL_INIT_VIDEO);
    LOG("SDL initialized");
    TTF_Init();
    LOG("TTF initialized");
    self->create_resources(self);
}

static void create_resources(UIClass *self)
{
    int32_t total_width = self->screen_width + 16 * 8 * self->scale;
    int32_t total_height = self->screen_height;
    SDL_CreateWindowAndRenderer(
        total_width, total_height, 0, &self->window, &self->renderer);
    self->screen =
        SDL_CreateRGBSurface(0, self->screen_width, self->screen_height, 32,
            0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    self->texture = SDL_CreateTexture(self->renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, self->screen_width, self->screen_height);
    self->debug_screen =
        SDL_CreateRGBSurface(0, 16 * 8 * self->scale, 32 * 8 * self->scale, 32,
            0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    self->debug_texture = SDL_CreateTexture(self->renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
        16 * 8 * self->scale, 32 * 8 * self->scale);
}

static void destructor(void *ptr)
{
    UIClass *self = (UIClass *) ptr;
    SDL_DestroyWindow(self->window);
    SDL_DestroyRenderer(self->renderer);
    SDL_DestroyTexture(self->texture);
    SDL_DestroyTexture(self->debug_texture);
    SDL_FreeSurface(self->debug_screen);
    SDL_FreeSurface(self->screen);
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
        if (event.type == SDL_KEYDOWN) {
            self->on_key(self, true, event.key.keysym.sym);
        }
        if (event.type == SDL_KEYUP) {
            self->on_key(self, false, event.key.keysym.sym);
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
            self->display_tile(self, tile_num, x_draw + (x * 8 * self->scale),
                y_draw + (y * 8 * self->scale));
            tile_num += 1;
        }
    }

    SDL_UpdateTexture(self->debug_texture, NULL, self->debug_screen->pixels,
        self->debug_screen->pitch);
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
    uint32_t *buffer = self->parent->ppu->context->video_buffer;

    for (int32_t line_num = 0; line_num < Y_RES; line_num++) {
        for (int32_t x = 0; x < X_RES; x++) {
            SDL_FillRect(self->screen,
                &(SDL_Rect){
                    .x = x * self->scale,
                    .y = line_num * self->scale,
                    .w = self->scale,
                    .h = self->scale,
                },
                buffer[x + (line_num * X_RES)]);
        }
    }

    SDL_UpdateTexture(
        self->texture, NULL, self->screen->pixels, self->screen->pitch);
    SDL_RenderClear(self->renderer);
    SDL_RenderCopy(self->renderer, self->texture, NULL,
        &(SDL_Rect){0, 0, self->screen_width, self->screen_height});
    self->update_debug_window(self);
    SDL_RenderCopy(self->renderer, self->debug_texture, NULL,
        &(SDL_Rect){self->screen_width, 0, 16 * 8 * self->scale,
            32 * 8 * self->scale});
    SDL_RenderPresent(self->renderer);
}

static uint32_t get_ticks(void)
{
    return SDL_GetTicks();
}

static void delay(uint32_t ms)
{
    return SDL_Delay(ms);
}

static void on_key(UIClass *self, bool down, SDL_Keycode code)
{
    switch (code) {
        case SDLK_a: {
            self->parent->joypad->context->state.a = down;
            break;
        }
        case SDLK_b: {
            self->parent->joypad->context->state.b = down;
            break;
        }
        case SDLK_RETURN: {
            self->parent->joypad->context->state.start = down;
            break;
        }
        case SDLK_TAB: {
            self->parent->joypad->context->state.select = down;
            break;
        }
        case SDLK_UP: {
            self->parent->joypad->context->state.up = down;
            break;
        }
        case SDLK_DOWN: {
            self->parent->joypad->context->state.down = down;
            break;
        }
        case SDLK_LEFT: {
            self->parent->joypad->context->state.left = down;
            break;
        }
        case SDLK_RIGHT: {
            self->parent->joypad->context->state.right = down;
            break;
        }
        case SDLK_q: {
            self->parent->context->die = true;
            break;
        }
    }
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
    .get_ticks = get_ticks,
    .delay = delay,
    .on_key = on_key,
};

const class_t *UI = (const class_t *) &init_ui;
