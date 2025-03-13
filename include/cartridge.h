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
    bool (*mbc_2)(CartridgeClass *);
    bool (*mbc_3)(CartridgeClass *);
    bool (*mbc_5)(CartridgeClass *);
    bool (*mbc_6)(CartridgeClass *);
    bool (*mbc_7)(CartridgeClass *);
    bool (*battery)(CartridgeClass *);
    bool (*rtc)(CartridgeClass *);
    void (*write_mbc1)(CartridgeClass *, uint16_t, uint8_t);
    void (*write_mbc2)(CartridgeClass *, uint16_t, uint8_t);
    void (*write_mbc3)(CartridgeClass *, uint16_t, uint8_t);
    void (*write_mbc5)(CartridgeClass *, uint16_t, uint8_t);
    void (*write_mbc6)(CartridgeClass *, uint16_t, uint8_t);
    void (*write_mbc7)(CartridgeClass *, uint16_t, uint8_t);
    void (*update_mbc6_banks)(CartridgeClass *);
    void (*handle_mbc7_transfer)(CartridgeClass *, uint8_t);
    uint8_t (*read_rtc)(CartridgeClass *, uint8_t);
    void (*write_rtc)(CartridgeClass *, uint8_t, uint8_t);
    void (*latch_rtc)(CartridgeClass *);
    void (*setup_banks)(CartridgeClass *);
    void (*load_battery)(CartridgeClass *);
    void (*save_battery)(CartridgeClass *);
} CartridgeClass;

extern const class_t *Cartridge;
#endif
