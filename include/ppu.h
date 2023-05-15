#include "common.h"
#include "oop.h"

#ifndef __PPU
    #define __PPU

typedef struct gameboy_aux GameboyClass;
typedef struct ppu_aux PPUClass;

typedef struct ppu_aux {
    /* Properties */
    class_t metadata;
    GameboyClass *parent;
    ppu_context_t *context;
    size_t target_time;
    size_t prev_time;
    size_t start_timer;
    size_t frame_count;
    /* Methods */
    void (*tick)(PPUClass *);
    void (*oam_write)(PPUClass *, uint16_t, uint8_t);
    uint8_t (*oam_read)(PPUClass *, uint16_t);
    void (*vram_write)(PPUClass *, uint16_t, uint8_t);
    uint8_t (*vram_read)(PPUClass *, uint16_t);
    /* State */
    void (*increment_y)(PPUClass *);
    void (*mode_hblank)(PPUClass *);
    void (*mode_vblank)(PPUClass *);
    void (*mode_oam)(PPUClass *);
    void (*mode_transfer)(PPUClass *);
} PPUClass;

extern const class_t *PPU;
#endif
