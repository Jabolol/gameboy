#include "common.h"
#include "oop.h"

#ifndef __PPU
    #define __PPU

typedef struct gameboy_aux GameboyClass;
typedef struct ppu_aux PPUClass;
typedef struct snapshot_aux SnapshotClass;

typedef struct ppu_aux {
    /* Properties */
    class_t metadata;
    GameboyClass *parent;
    ppu_context_t *context;
    uint64_t frame_deadline;
    uint64_t frame_duration;
    uint32_t frame_count;
    /* Methods */
    void (*tick)(PPUClass *, uint32_t);
    void (*oam_write)(PPUClass *, uint16_t, uint8_t);
    uint8_t (*oam_read)(PPUClass *, uint16_t);
    void (*vram_write)(PPUClass *, uint16_t, uint8_t);
    uint8_t (*vram_read)(PPUClass *, uint16_t);
    /* State */
    void (*set_mode)(PPUClass *, lcd_mode_t);
    void (*increment_y)(PPUClass *);
    void (*mode_hblank)(PPUClass *);
    void (*mode_vblank)(PPUClass *);
    void (*mode_oam)(PPUClass *);
    void (*mode_transfer)(PPUClass *);
    void (*update_stat)(PPUClass *);
    void (*check_lyc)(PPUClass *);
    void (*stat_write_quirk)(PPUClass *);
    void (*lcd_on)(PPUClass *);
    void (*lcd_off)(PPUClass *);
    void (*sync)(PPUClass *);
    uint32_t (*transfer_length)(PPUClass *);
    void (*frame_complete)(PPUClass *);
    void (*end_line)(PPUClass *);
    void (*process_event)(PPUClass *);
    bool (*oam_accessible)(PPUClass *, bool);
    bool (*vram_accessible)(PPUClass *, bool);
    void (*block_access)(PPUClass *, bool, bool, bool, bool);
    /* Sprites */
    void (*scan_oam)(PPUClass *, uint8_t);
    void (*load_line_sprites)(PPUClass *);
    void (*serialize)(PPUClass *, SnapshotClass *);
} PPUClass;

extern const class_t *PPU;
#endif
