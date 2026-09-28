#include "common.h"
#include "oop.h"

#ifndef __RAM
    #define __RAM

typedef struct ram_aux RAMClass;
typedef struct snapshot_aux SnapshotClass;

typedef struct ram_aux {
    /* Properties */
    class_t metadata;
    ram_context_t *context;
    /* Methods */
    uint8_t (*wram_read)(RAMClass *, uint16_t);
    void (*wram_write)(RAMClass *, uint16_t, uint8_t);
    uint8_t (*hram_read)(RAMClass *, uint16_t);
    void (*hram_write)(RAMClass *, uint16_t, uint8_t);
    void (*serialize)(RAMClass *, SnapshotClass *);
} RAMClass;

extern const class_t *RAM;
#endif
