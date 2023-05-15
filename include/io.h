#include "common.h"
#include "oop.h"

#ifndef __IO
    #define __IO

typedef struct gameboy_aux GameboyClass;
typedef struct io_aux IOClass;

typedef struct io_aux {
    /* Properties */
    class_t metadata;
    GameboyClass *parent;
    char serial_data[2];
    /* Methods */
    uint8_t (*read)(IOClass *, uint16_t);
    void (*write)(IOClass *, uint16_t, uint8_t);
} IOClass;

extern const class_t *IO;
#endif
