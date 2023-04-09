#include "common.h"
#include "oop.h"

#ifndef __CARTRIDGE
    #define __CARTRIDGE
typedef struct cartridge_aux CartridgeClass;

typedef struct cartridge_aux {
    /* Properties */
    class_t metadata;
    const char *rom_types[35];
    const char *license_codes[0xA5];
    cartridge_context_t *context;
    /* Methods */
    const char *(*get_license)(CartridgeClass *);
    const char *(*get_rom_type)(CartridgeClass *);
    bool (*load)(CartridgeClass *, char *);
} CartridgeClass;

extern const class_t *Cartridge;
#endif
