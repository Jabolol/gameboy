#include "cartridge.h"
#include "common.h"
#include "oop.h"

#ifndef __BUS
    #define __BUS
typedef struct bus_aux BusClass;

typedef struct bus_aux {
    /* Properties */
    class_t metadata;
    CartridgeClass *cartridge;
    /* Methods */
    uint8_t (*read)(BusClass *, uint16_t);
    void (*write)(BusClass *, uint16_t, uint8_t);
} BusClass;

extern const class_t *Bus;
#endif
