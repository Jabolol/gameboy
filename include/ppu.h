#include "common.h"
#include "oop.h"

#ifndef __PPU
    #define __PPU

typedef struct ppu_aux PPUClass;

typedef struct ppu_aux {
    /* Properties */
    class_t metadata;
    ppu_context_t *context;
    /* Methods */
    void (*tick)(void);
    void (*oam_write)(PPUClass *, uint16_t, uint8_t);
    uint8_t (*oam_read)(PPUClass *, uint16_t);
    void (*vram_write)(PPUClass *, uint16_t, uint8_t);
    uint8_t (*vram_read)(PPUClass *, uint16_t);
} PPUClass;

extern const class_t *PPU;
#endif
