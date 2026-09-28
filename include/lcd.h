#include "common.h"
#include "oop.h"

#ifndef __LCD
    #define __LCD

typedef struct gameboy_aux GameboyClass;
typedef struct lcd_aux LCDClass;
typedef struct snapshot_aux SnapshotClass;

typedef struct lcd_aux {
    /* Properties */
    class_t metadata;
    GameboyClass *parent;
    const uint32_t dmg_palettes[PALETTE_COUNT][4];
    lcd_context_t *context;
    /* Methods */
    uint8_t (*read)(LCDClass *, uint16_t);
    void (*write)(LCDClass *, uint16_t, uint8_t);
    void (*update)(LCDClass *, uint8_t, uint8_t);
    void (*update_cgb_color)(LCDClass *, bool, uint8_t);
    void (*refresh_colors)(LCDClass *);
    uint8_t (*read_status)(LCDClass *);
    void (*hdma_start)(LCDClass *, uint8_t);
    void (*hdma_block)(LCDClass *);
    void (*hdma_hblank)(LCDClass *);
    void (*serialize)(LCDClass *, SnapshotClass *);
} LCDClass;

extern const class_t *LCD;
#endif
