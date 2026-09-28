#include "common.h"
#include "oop.h"

#ifndef __IO
    #define __IO

typedef struct gameboy_aux GameboyClass;
typedef struct io_aux IOClass;
typedef struct snapshot_aux SnapshotClass;

typedef struct io_aux {
    /* Properties */
    class_t metadata;
    GameboyClass *parent;
    serial_context_t *serial;
    /* Methods */
    uint8_t (*read)(IOClass *, uint16_t);
    void (*write)(IOClass *, uint16_t, uint8_t);
    void (*tick)(IOClass *);
    void (*serial_start)(IOClass *, uint8_t);
    void (*serialize)(IOClass *, SnapshotClass *);
} IOClass;

extern const class_t *IO;
#endif
