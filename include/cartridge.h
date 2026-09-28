#include "common.h"
#include "oop.h"

#ifndef __CARTRIDGE
    #define __CARTRIDGE

typedef struct gameboy_aux GameboyClass;
typedef struct cartridge_aux CartridgeClass;
typedef struct snapshot_aux SnapshotClass;

typedef struct cartridge_aux {
    /* Properties */
    class_t metadata;
    GameboyClass *parent;
    const cartridge_type_t types[0x100];
    const char *license_codes[0xA5];
    char title[17];
    cartridge_context_t *context;
    /* Methods */
    const char *(*get_license)(CartridgeClass *);
    const char *(*get_rom_type)(CartridgeClass *);
    bool (*load)(CartridgeClass *, const char *, const char *);
    uint8_t (*read)(CartridgeClass *, uint16_t);
    void (*write)(CartridgeClass *, uint16_t, uint8_t);
    void (*detect_mbc)(CartridgeClass *);
    void (*setup_banks)(CartridgeClass *);
    void (*update_mapping)(CartridgeClass *);
    void (*serialize)(CartridgeClass *, SnapshotClass *);
    void (*map_rom)(CartridgeClass *, uint8_t, uint32_t);
    void (*map_ram)(CartridgeClass *, uint32_t, bool, bool);
    void (*unmap_ram)(CartridgeClass *);
    /* Controller */
    void (*mbc_write)(CartridgeClass *, uint16_t, uint8_t);
    uint8_t (*ram_read)(CartridgeClass *, uint16_t);
    void (*ram_write)(CartridgeClass *, uint16_t, uint8_t);
    void (*write_mbc1)(CartridgeClass *, uint16_t, uint8_t);
    void (*write_mbc2)(CartridgeClass *, uint16_t, uint8_t);
    void (*write_mbc3)(CartridgeClass *, uint16_t, uint8_t);
    void (*write_mbc5)(CartridgeClass *, uint16_t, uint8_t);
    void (*write_mbc6)(CartridgeClass *, uint16_t, uint8_t);
    void (*write_mbc7)(CartridgeClass *, uint16_t, uint8_t);
    void (*write_mmm01)(CartridgeClass *, uint16_t, uint8_t);
    void (*write_huc1)(CartridgeClass *, uint16_t, uint8_t);
    void (*write_huc3)(CartridgeClass *, uint16_t, uint8_t);
    void (*write_camera)(CartridgeClass *, uint16_t, uint8_t);
    void (*write_wisdom_tree)(CartridgeClass *, uint16_t, uint8_t);
    void (*update_mbc1)(CartridgeClass *);
    void (*update_banks)(CartridgeClass *);
    void (*update_mbc6)(CartridgeClass *);
    void (*update_mmm01)(CartridgeClass *);
    void (*update_huc)(CartridgeClass *);
    void (*update_camera)(CartridgeClass *);
    uint8_t (*read_mbc2_ram)(CartridgeClass *, uint16_t);
    void (*write_mbc2_ram)(CartridgeClass *, uint16_t, uint8_t);
    uint8_t (*read_mbc3_ram)(CartridgeClass *, uint16_t);
    void (*write_mbc3_ram)(CartridgeClass *, uint16_t, uint8_t);
    uint8_t (*read_mbc7_ram)(CartridgeClass *, uint16_t);
    void (*write_mbc7_ram)(CartridgeClass *, uint16_t, uint8_t);
    uint8_t (*read_huc_ram)(CartridgeClass *, uint16_t);
    void (*write_huc_ram)(CartridgeClass *, uint16_t, uint8_t);
    uint8_t (*read_camera_ram)(CartridgeClass *, uint16_t);
    void (*write_camera_ram)(CartridgeClass *, uint16_t, uint8_t);
    uint8_t (*read_open_bus)(CartridgeClass *, uint16_t);
    void (*write_ignored)(CartridgeClass *, uint16_t, uint8_t);
    /* Peripherals */
    void (*flash_command)(CartridgeClass *, uint8_t, uint16_t, uint8_t);
    void (*eeprom_write)(CartridgeClass *, uint8_t);
    void (*eeprom_command)(CartridgeClass *);
    void (*rtc_update)(CartridgeClass *);
    void (*rtc_advance)(CartridgeClass *, uint64_t);
    void (*latch_rtc)(CartridgeClass *);
    uint8_t (*read_rtc)(CartridgeClass *, uint8_t);
    void (*write_rtc)(CartridgeClass *, uint8_t, uint8_t);
    void (*huc3_update)(CartridgeClass *);
    void (*huc3_execute)(CartridgeClass *);
    void (*load_battery)(CartridgeClass *);
    size_t (*battery_trailer)(CartridgeClass *, uint8_t *);
    void (*save_battery)(CartridgeClass *);
} CartridgeClass;

extern const class_t *Cartridge;
#endif
