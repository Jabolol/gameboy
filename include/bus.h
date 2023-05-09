#include "cartridge.h"
#include "common.h"
#include "oop.h"
#include "ram.h"

#ifndef __BUS
    #define __BUS

typedef struct bus_aux BusClass;
typedef struct gameboy_aux GameboyClass;

typedef struct bus_aux {
    /* Properties */
    class_t metadata;
    GameboyClass *parent;
    /* Methods */
    uint8_t (*read)(BusClass *, uint16_t);
    void (*write)(BusClass *, uint16_t, uint8_t);
    uint16_t (*read16)(BusClass *, uint16_t);
    void (*write16)(BusClass *, uint16_t, uint16_t);
} BusClass;

extern const class_t *Bus;
#endif
