#include "../include/gameboy.h"

#define GAME_WIDTH    (X_RES * 2)
#define GAME_HEIGHT   (Y_RES * 2)
#define PANEL_WIDTH   256
#define PANEL_HEIGHT  GAME_HEIGHT
#define PANEL_TILES   384
#define SWATCH_WIDTH  8
#define SWATCH_HEIGHT 16
#define AXIS_DEADZONE 16000
#ifdef __EMSCRIPTEN__
    #define SDL_FLAGS    SDL_INIT_VIDEO
    #define WINDOW_FLAGS 0
#else
    #define SDL_FLAGS    (SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER)
    #define WINDOW_FLAGS SDL_WINDOW_RESIZABLE
#endif

static void constructor(void *ptr, va_list *args)
{
    UIClass *self = (UIClass *) ptr;
    self->parent = va_arg(*args, GameboyClass *);
    self->scale = va_arg(*args, int32_t);
    self->panel_visible = true;
    if (!((self->panel_pixels =
                calloc(PANEL_WIDTH * PANEL_HEIGHT, sizeof(uint32_t))))) {
        HANDLE_ERROR("failed memory allocation");
    }
    SDL_Init(SDL_FLAGS);
    LOG("SDL initialized");
    self->create_resources(self);
}

static void create_resources(UIClass *self)
{
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
#ifdef __EMSCRIPTEN__
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
#endif
    SDL_CreateWindowAndRenderer((GAME_WIDTH + PANEL_WIDTH) * self->scale,
        GAME_HEIGHT * self->scale, WINDOW_FLAGS, &self->window,
        &self->renderer);
    SDL_RenderSetIntegerScale(self->renderer, SDL_TRUE);
    SDL_RenderSetLogicalSize(
        self->renderer, GAME_WIDTH + PANEL_WIDTH, GAME_HEIGHT);
    self->texture = SDL_CreateTexture(self->renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, X_RES, Y_RES);
    self->panel_texture =
        SDL_CreateTexture(self->renderer, SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING, PANEL_WIDTH, PANEL_HEIGHT);
}

static void destructor(void *ptr)
{
    UIClass *self = (UIClass *) ptr;
    if (self->controller) {
        SDL_GameControllerClose(self->controller);
    }
    SDL_DestroyTexture(self->texture);
    SDL_DestroyTexture(self->panel_texture);
    SDL_DestroyRenderer(self->renderer);
    SDL_DestroyWindow(self->window);
    free(self->panel_pixels);
    SDL_Quit();
}

static int32_t panel_width(UIClass *self)
{
    return self->parent->context->hw_mode == HW_CGB ? PANEL_WIDTH
                                                    : PANEL_WIDTH / 2;
}

static void resize(UIClass *self)
{
    int32_t width =
        GAME_WIDTH + (self->panel_visible ? self->panel_width(self) : 0);

    SDL_RenderSetLogicalSize(self->renderer, width, GAME_HEIGHT);
    SDL_SetWindowSize(
        self->window, width * self->scale, GAME_HEIGHT * self->scale);
}

static void handle_events(UIClass *self)
{
    SDL_Event event = {0};

    while (SDL_PollEvent(&event) > 0) {
        switch (event.type) {
            case SDL_QUIT:
                atomic_store(&self->parent->context->die, true);
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                if (!event.key.repeat) {
                    self->on_key(
                        self, event.type == SDL_KEYDOWN, event.key.keysym.sym);
                }
                break;
            }
            case SDL_CONTROLLERDEVICEADDED: {
                if (!self->controller) {
                    self->controller =
                        SDL_GameControllerOpen(event.cdevice.which);
                }
                break;
            }
            case SDL_CONTROLLERDEVICEREMOVED: {
                SDL_GameControllerClose(self->controller);
                self->controller = NULL;
                break;
            }
            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP: {
                self->on_controller(self,
                    event.type == SDL_CONTROLLERBUTTONDOWN,
                    event.cbutton.button);
                break;
            }
            case SDL_CONTROLLERAXISMOTION: {
                self->on_axis(self, event.caxis.axis, event.caxis.value);
                break;
            }
        }
    }
}

static void assign_palettes(UIClass *self)
{
    lcd_context_t *lcd = self->parent->lcd->context;
    ppu_context_t *ppu = self->parent->ppu->context;
    bool cgb = self->parent->context->hw_mode == HW_CGB;
    uint16_t maps[2] = {(lcd->control & 0x08) ? 0x1C00 : 0x1800,
        (lcd->control & 0x40) ? 0x1C00 : 0x1800};

    for (uint16_t tile = 0; tile < PANEL_TILES; tile++) {
        self->tile_palettes[0][tile] =
            cgb ? lcd->bg_colors_cgb[0] : lcd->bg_colors;
        self->tile_palettes[1][tile] = self->tile_palettes[0][tile];
    }
    for (uint8_t map = 0; map < ((lcd->control & 0x20) ? 2 : 1); map++) {
        for (uint16_t i = 0; i < 0x400; i++) {
            uint8_t index = ppu->vram[maps[map] + i];
            uint8_t attributes = cgb ? ppu->vram[0x2000 + maps[map] + i] : 0;
            uint16_t tile =
                (lcd->control & 0x10) || index >= 128 ? index : 256 + index;
            self->tile_palettes[(attributes >> 3) & 1][tile] =
                cgb ? lcd->bg_colors_cgb[attributes & 0x07] : lcd->bg_colors;
        }
    }
    for (uint8_t i = 0; i < OAM_ENTRIES; i++) {
        oam_entry_t *entry = &ppu->oam_ram[i];
        uint8_t bank = cgb && (entry->attributes & 0x08);
        const uint32_t *colors = cgb
            ? lcd->sprite_colors_cgb[entry->attributes & 0x07]
            : ((entry->attributes & 0x10) ? lcd->sprite2_colors
                                          : lcd->sprite1_colors);

        if (!entry->y || entry->y >= 160 || !entry->x || entry->x >= 168) {
            continue;
        }
        self->tile_palettes[bank][entry->tile] = colors;
        if (lcd->control & 0x04) {
            self->tile_palettes[bank][entry->tile ^ 1] = colors;
        }
    }
}

static void display_tile(
    UIClass *self, uint8_t bank, uint16_t tile, int32_t x, int32_t y)
{
    uint8_t *data =
        self->parent->ppu->context->vram + (bank * 0x2000) + (tile * 16);
    const uint32_t *colors = self->tile_palettes[bank][tile];

    for (int32_t row = 0; row < 8; row++) {
        uint32_t *pixels = self->panel_pixels + ((y + row) * PANEL_WIDTH) + x;
        for (int32_t bit = 7; bit >= 0; bit--) {
            uint8_t color = (((data[row * 2 + 1] >> bit) & 1) << 1)
                | ((data[row * 2] >> bit) & 1);
            pixels[7 - bit] = colors[color];
        }
    }
}

static void display_palettes(UIClass *self)
{
    lcd_context_t *lcd = self->parent->lcd->context;
    bool cgb = self->parent->context->hw_mode == HW_CGB;
    const uint32_t *rows[2][8] = {
        {lcd->bg_colors}, {lcd->sprite1_colors, lcd->sprite2_colors}};

    if (cgb) {
        for (uint8_t i = 0; i < 8; i++) {
            rows[0][i] = lcd->bg_colors_cgb[i];
            rows[1][i] = lcd->sprite_colors_cgb[i];
        }
    }
    for (uint8_t row = 0; row < 2; row++) {
        for (uint8_t swatch = 0; swatch < 32; swatch++) {
            const uint32_t *palette = rows[row][swatch / 4];
            int32_t top =
                (PANEL_TILES / 16) * 8 + 16 + row * (SWATCH_HEIGHT + 8);
            if (!palette) {
                continue;
            }
            for (int32_t y = top; y < top + SWATCH_HEIGHT; y++) {
                for (int32_t x = 0; x < SWATCH_WIDTH; x++) {
                    self->panel_pixels[(y * PANEL_WIDTH)
                        + (swatch * SWATCH_WIDTH) + x] = palette[swatch % 4];
                }
            }
        }
    }
}

static void update_panel(UIClass *self)
{
    uint8_t banks = self->parent->context->hw_mode == HW_CGB ? 2 : 1;

    for (uint32_t i = 0; i < PANEL_WIDTH * PANEL_HEIGHT; i++) {
        self->panel_pixels[i] = 0xFF111111;
    }
    self->assign_palettes(self);
    for (uint8_t bank = 0; bank < banks; bank++) {
        for (uint16_t tile = 0; tile < PANEL_TILES; tile++) {
            self->display_tile(self, bank, tile,
                (bank * 128) + (tile % 16) * 8, (tile / 16) * 8);
        }
    }
    self->display_palettes(self);
    SDL_UpdateTexture(self->panel_texture, NULL, self->panel_pixels,
        PANEL_WIDTH * sizeof(uint32_t));
}

static void update(UIClass *self)
{
    SDL_UpdateTexture(self->texture, NULL,
        self->parent->ppu->context->video_buffer, X_RES * sizeof(uint32_t));
    SDL_RenderClear(self->renderer);
    SDL_RenderCopy(self->renderer, self->texture, NULL,
        &(SDL_Rect) {0, 0, GAME_WIDTH, GAME_HEIGHT});
    if (self->panel_visible) {
        int32_t width = self->panel_width(self);

        self->update_panel(self);
        SDL_RenderCopy(self->renderer, self->panel_texture,
            &(SDL_Rect) {0, 0, width, PANEL_HEIGHT},
            &(SDL_Rect) {GAME_WIDTH, 0, width, PANEL_HEIGHT});
    }
    SDL_RenderPresent(self->renderer);
    self->update_rumble(self);
}

static void update_rumble(UIClass *self)
{
    bool rumble = self->parent->cartridge->context->rumble;

    if (self->controller && (rumble || self->rumbling)) {
        SDL_GameControllerRumble(self->controller, rumble ? 0xFFFF : 0,
            rumble ? 0xFFFF : 0, rumble ? 100 : 0);
    }
    self->rumbling = rumble;
}

static uint32_t get_ticks(void)
{
    return SDL_GetTicks();
}

static void delay(uint32_t ms)
{
    SDL_Delay(ms);
}

static void on_key(UIClass *self, bool down, SDL_Keycode code)
{
    GameboyClass *gameboy = self->parent;
    JoypadClass *joypad = gameboy->joypad;

    switch (code) {
        case SDLK_z: joypad->set_button(joypad, BUTTON_A, down); break;
        case SDLK_x: joypad->set_button(joypad, BUTTON_B, down); break;
        case SDLK_RETURN:
            joypad->set_button(joypad, BUTTON_START, down);
            break;
        case SDLK_TAB: joypad->set_button(joypad, BUTTON_SELECT, down); break;
        case SDLK_UP: joypad->set_button(joypad, BUTTON_UP, down); break;
        case SDLK_DOWN: joypad->set_button(joypad, BUTTON_DOWN, down); break;
        case SDLK_LEFT: joypad->set_button(joypad, BUTTON_LEFT, down); break;
        case SDLK_RIGHT: joypad->set_button(joypad, BUTTON_RIGHT, down); break;
#ifndef __EMSCRIPTEN__
        case SDLK_SPACE:
            gameboy->configure(gameboy, SETTING_TURBO, down);
            break;
#endif
        default: break;
    }
    if (!down) {
        return;
    }
    switch (code) {
        case SDLK_s: gameboy->request(gameboy, REQUEST_SAVE_STATE); break;
        case SDLK_l: gameboy->request(gameboy, REQUEST_LOAD_STATE); break;
#ifndef __EMSCRIPTEN__
        case SDLK_p: {
            gameboy->configure(gameboy, SETTING_PALETTE,
                atomic_load(&gameboy->context->palette) + 1);
            break;
        }
        case SDLK_c: {
            gameboy->configure(gameboy, SETTING_COLOR_CORRECTION,
                !atomic_load(&gameboy->context->color_correction));
            break;
        }
        case SDLK_t: {
            self->panel_visible = !self->panel_visible;
            self->resize(self);
            break;
        }
        case SDLK_u:
        case SDLK_d:
            gameboy->sound->update_volume(gameboy->sound, code == SDLK_u);
            break;
        case SDLK_q: atomic_store(&gameboy->context->die, true); break;
#endif
        default: break;
    }
}

static void on_controller(UIClass *self, bool down, uint8_t button)
{
    GameboyClass *gameboy = self->parent;
    JoypadClass *joypad = gameboy->joypad;

    switch (button) {
        case SDL_CONTROLLER_BUTTON_B:
            joypad->set_button(joypad, BUTTON_A, down);
            break;
        case SDL_CONTROLLER_BUTTON_A:
            joypad->set_button(joypad, BUTTON_B, down);
            break;
        case SDL_CONTROLLER_BUTTON_START:
            joypad->set_button(joypad, BUTTON_START, down);
            break;
        case SDL_CONTROLLER_BUTTON_BACK:
            joypad->set_button(joypad, BUTTON_SELECT, down);
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            joypad->set_button(joypad, BUTTON_UP, down);
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            joypad->set_button(joypad, BUTTON_DOWN, down);
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            joypad->set_button(joypad, BUTTON_LEFT, down);
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            joypad->set_button(joypad, BUTTON_RIGHT, down);
            break;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
            gameboy->configure(gameboy, SETTING_TURBO, down);
            break;
        default: break;
    }
}

static void on_axis(UIClass *self, uint8_t axis, int16_t value)
{
    JoypadClass *joypad = self->parent->joypad;

    if (axis == SDL_CONTROLLER_AXIS_LEFTX) {
        joypad->set_button(joypad, BUTTON_LEFT, value < -AXIS_DEADZONE);
        joypad->set_button(joypad, BUTTON_RIGHT, value > AXIS_DEADZONE);
    } else if (axis == SDL_CONTROLLER_AXIS_LEFTY) {
        joypad->set_button(joypad, BUTTON_UP, value < -AXIS_DEADZONE);
        joypad->set_button(joypad, BUTTON_DOWN, value > AXIS_DEADZONE);
    }
}

const UIClass init_ui = {
    {
        ._size = sizeof(UIClass),
        ._name = "UI",
        ._constructor = constructor,
        ._destructor = destructor,
    },
    .create_resources = create_resources,
    .panel_width = panel_width,
    .resize = resize,
    .handle_events = handle_events,
    .assign_palettes = assign_palettes,
    .display_tile = display_tile,
    .display_palettes = display_palettes,
    .update_panel = update_panel,
    .update = update,
    .update_rumble = update_rumble,
    .get_ticks = get_ticks,
    .delay = delay,
    .on_key = on_key,
    .on_controller = on_controller,
    .on_axis = on_axis,
};

const class_t *UI = (const class_t *) &init_ui;
