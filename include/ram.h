#include "common.h"
#include "oop.h"

#ifndef __RAM
    #define __RAM

typedef struct ram_aux RamClass;

typedef struct ram_aux {
    /* Properties */
    class_t metadata;
    ram_context_t *context;
    /* Methods */
    uint8_t (*wram_read)(RamClass *, uint16_t);
    void (*wram_write)(RamClass *, uint16_t, uint8_t);
    uint8_t (*hram_read)(RamClass *, uint16_t);
    void (*hram_write)(RamClass *, uint16_t, uint8_t);
} RamClass;

extern const class_t *Ram;
#endif
