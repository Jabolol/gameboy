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
    uint16_t set_banks;
    cartridge_context_t *context;
    /* Methods */
    const char *(*get_license)(CartridgeClass *);
    const char *(*get_rom_type)(CartridgeClass *);
    bool (*load)(CartridgeClass *, char *);
    uint8_t (*read)(CartridgeClass *, uint16_t);
    void (*write)(CartridgeClass *, uint16_t, uint8_t);
    bool (*mbc_1)(CartridgeClass *);
    bool (*battery)(CartridgeClass *);
    void (*setup_banks)(CartridgeClass *);
    void (*load_battery)(CartridgeClass *);
    void (*save_battery)(CartridgeClass *);
} CartridgeClass;

extern const class_t *Cartridge;
#endif
