#include "../include/gameboy.h"

static void constructor(void *ptr, va_list UNUSED *args)
{
    CartridgeClass *self = (CartridgeClass *) ptr;
    if (!((self->context = calloc(1, sizeof(*self->context))))) {
        HANDLE_ERROR("failed memory allocation");
    }
}

static void destructor(void *ptr)
{
    CartridgeClass *self = (CartridgeClass *) ptr;

    for (int i = 0; i < 16; i++) {
        if (self->set_banks & (1 << i)) {
            free(self->context->ram_banks[i]);
        }
    }

    free(self->context->rom_data);
    free(self->context);
}

static const char *get_license(CartridgeClass *self)
{
    return self->context->header->new_license_code <= 0xA4
        ? self->license_codes[self->context->header->license_code]
        : "UNKNOWN";
}

static const char *get_rom_type(CartridgeClass *self)
{
    return self->context->header->type <= 0x22
        ? self->rom_types[self->context->header->type]
        : "UNKNOWN";
}

static bool load(CartridgeClass *self, char *path)
{
    strncpy(self->context->filename, path, sizeof(self->context->filename));
    FILE *stream = fopen(path, "r");
    if (!stream) {
        fprintf(stderr, "Failed to open ROM (%s)\n", path);
        return false;
    }
    printf("Opened: %s\n", path);
    fseek(stream, 0, SEEK_END);
    self->context->rom_size = ftell(stream);
    rewind(stream);

    if (!((self->context->rom_data = calloc(self->context->rom_size, 1)))) {
        fclose(stream);
        HANDLE_ERROR("failed memory allocation");
    }
    fread(self->context->rom_data, self->context->rom_size, 1, stream);

    self->context->header = (rom_header_t *) (self->context->rom_data + 0x100);
    self->context->header->title[15] = 0;
    self->context->has_battery = self->battery(self);
    self->context->needs_save = false;
    fclose(stream);

    printf("Cartridge loaded:\n");
    printf("\tTitle    : %s\n", self->context->header->title);
    printf("\tType     : %2.2X (%s)\n", self->context->header->type,
        self->get_rom_type(self));
    printf("\tROM Size : %d KB\n", 32 << self->context->header->rom_size);
    printf("\tRAM Size : %2.2X KB\n", self->context->header->ram_size);
    printf("\tLIC Code : %2.2X (%s)\n", self->context->header->license_code,
        self->get_license(self));
    printf("\tROM Vers : %2.2X\n", self->context->header->version);

    self->setup_banks(self);

    uint16_t x = 0;
    for (uint16_t i = 0x0134; i <= 0x014c; i++) {
        x -= self->context->rom_data[i] - 1;
    }

    printf("\tChecksum : %2.2X (%s)\n", self->context->header->checksum,
        (x & 0xFF) ? "PASSED" : "FAILED");

    if (self->context->has_battery) {
        self->load_battery(self);
    }

    return true;
}

static uint8_t read(CartridgeClass *self, uint16_t address)
{
    if (!self->mbc_1(self) || address < 0x4000) {
        return self->context->rom_data[address];
    }

    if ((address & 0xE000) == 0xA000) {
        if (!self->context->ram_enabled) {
            return 0xFF;
        }

        if (self->context->ram_bank == NULL) {
            return 0xFF;
        }

        return self->context->ram_bank[address - 0xA000];
    }

    return self->context->rom_bank_x[address - 0x4000];
}

static void write(CartridgeClass *self, uint16_t address, uint8_t value)
{
    if (!self->mbc_1(self)) {
        return;
    }

    switch (address & 0xE000) {
        case 0x0000: {
            self->context->ram_enabled = (value & 0x0F) == 0x0A;
            break;
        }
        case 0x2000: {
            if (value == 0) {
                value = 1;
            }
            value &= 0x1F;
            self->context->rom_bank_value = value;
            self->context->rom_bank_x = self->context->rom_data
                + (0x4000 * self->context->rom_bank_value);
            break;
        }
        case 0x4000: {
            self->context->ram_bank_value = value & 0x03;
            if (self->context->ram_banking) {
                if (self->context->needs_save) {
                    self->save_battery(self);
                }
                self->context->ram_bank =
                    self->context->ram_banks[self->context->ram_bank_value];
            }
            break;
        }
        case 0x6000: {
            self->context->banking_mode = value & 0x01;
            self->context->ram_banking = self->context->banking_mode;
            if (self->context->ram_banking) {
                if (self->context->needs_save) {
                    self->save_battery(self);
                }
                self->context->ram_bank =
                    self->context->ram_banks[self->context->ram_bank_value];
            }
            break;
        }
        case 0xA000: {
            if (!self->context->ram_enabled) {
                return;
            }
            if (self->context->ram_bank == NULL) {
                return;
            }
            self->context->ram_bank[address - 0xA000] = value;
            if (self->context->has_battery) {
                self->context->needs_save = true;
            }
            break;
        }
    }
}

static bool mbc_1(CartridgeClass *self)
{
    return BETWEEN(self->context->header->type, 0x01, 0x03);
}

static bool battery(CartridgeClass *self)
{
    return self->context->header->type == 0x03;
}

static void setup_banks(CartridgeClass *self)
{
    self->set_banks = 0;

    for (int i = 0; i < 16; i++) {
        self->context->ram_banks[i] = NULL;

        if (self->context->header->ram_size == 2 && i == 0) {
            self->context->ram_banks[i] = calloc(0x2000, 1);
            self->set_banks |= 1 << i;
        }
        if (self->context->header->ram_size == 3 && i < 4) {
            self->context->ram_banks[i] = calloc(0x2000, 1);
            self->set_banks |= 1 << i;
        }
        if (self->context->header->ram_size == 4 && i < 16) {
            self->context->ram_banks[i] = calloc(0x2000, 1);
            self->set_banks |= 1 << i;
        }
        if (self->context->header->ram_size == 5 && i < 8) {
            self->context->ram_banks[i] = calloc(0x2000, 1);
            self->set_banks |= 1 << i;
        }
    }

    self->context->ram_bank = self->context->ram_banks[0];
    self->context->rom_bank_x = self->context->rom_data + 0x4000;
}

static void load_battery(CartridgeClass *self)
{
    if (self->context->ram_bank == NULL) {
        return;
    }

    char name[1030] = {0};
    snprintf(name, sizeof(name), "%s.sav", self->context->filename);

    FILE *stream = fopen(name, "rb");
    if (!stream) {
        LOG("Failed to open battery file");
        return;
    }

    fread(self->context->ram_bank, 0x2000, 1, stream);
    fclose(stream);
}

static void save_battery(CartridgeClass *self)
{
    if (self->context->ram_bank == NULL) {
        return;
    }

    char name[1030] = {0};
    snprintf(name, sizeof(name), "%s.sav", self->context->filename);

    FILE *stream = fopen(name, "wb");
    if (!stream) {
        LOG("Failed to open battery file");
        return;
    }

    fwrite(self->context->ram_bank, 0x2000, 1, stream);
    fclose(stream);
}

const CartridgeClass init_cartridge = {
    {
        ._size = sizeof(CartridgeClass),
        ._name = "Cartridge",
        ._constructor = constructor,
        ._destructor = destructor,
    },
    .rom_types =
        {
            "ROM ONLY",
            "MBC1",
            "MBC1+RAM",
            "MBC1+RAM+BATTERY",
            "0x04 ???",
            "MBC2",
            "MBC2+BATTERY",
            "0x07 ???",
            "ROM+RAM 1",
            "ROM+RAM+BATTERY 1",
            "0x0A ???",
            "MMM01",
            "MMM01+RAM",
            "MMM01+RAM+BATTERY",
            "0x0E ???",
            "MBC3+TIMER+BATTERY",
            "MBC3+TIMER+RAM+BATTERY 2",
            "MBC3",
            "MBC3+RAM 2",
            "MBC3+RAM+BATTERY 2",
            "0x14 ???",
            "0x15 ???",
            "0x16 ???",
            "0x17 ???",
            "0x18 ???",
            "MBC5",
            "MBC5+RAM",
            "MBC5+RAM+BATTERY",
            "MBC5+RUMBLE",
            "MBC5+RUMBLE+RAM",
            "MBC5+RUMBLE+RAM+BATTERY",
            "0x1F ???",
            "MBC6",
            "0x21 ???",
            "MBC7+SENSOR+RUMBLE+RAM+BATTERY",
        },
    .license_codes =
        {
            [0x00] = "None",
            [0x01] = "Nintendo R&D1",
            [0x08] = "Capcom",
            [0x13] = "Electronic Arts",
            [0x18] = "Hudson Soft",
            [0x19] = "b-ai",
            [0x20] = "kss",
            [0x22] = "pow",
            [0x24] = "PCM Complete",
            [0x25] = "san-x",
            [0x28] = "Kemco Japan",
            [0x29] = "seta",
            [0x30] = "Viacom",
            [0x31] = "Nintendo",
            [0x32] = "Bandai",
            [0x33] = "Ocean/Acclaim",
            [0x34] = "Konami",
            [0x35] = "Hector",
            [0x37] = "Taito",
            [0x38] = "Hudson",
            [0x39] = "Banpresto",
            [0x41] = "Ubisoft",
            [0x42] = "Atlus",
            [0x44] = "Malibu",
            [0x46] = "angel",
            [0x47] = "Bullet-Proof",
            [0x49] = "irem",
            [0x50] = "Absolute",
            [0x51] = "Acclaim",
            [0x52] = "Activision",
            [0x53] = "American sammy",
            [0x54] = "Konami",
            [0x55] = "Hi tech entertainment",
            [0x56] = "LJN",
            [0x57] = "Matchbox",
            [0x58] = "Mattel",
            [0x59] = "Milton Bradley",
            [0x60] = "Titus",
            [0x61] = "Virgin",
            [0x64] = "LucasArts",
            [0x67] = "Ocean",
            [0x69] = "Electronic Arts",
            [0x70] = "Infogrames",
            [0x71] = "Interplay",
            [0x72] = "Broderbund",
            [0x73] = "sculptured",
            [0x75] = "sci",
            [0x78] = "THQ",
            [0x79] = "Accolade",
            [0x80] = "misawa",
            [0x83] = "lozc",
            [0x86] = "Tokuma Shoten Intermedia",
            [0x87] = "Tsukuda Original",
            [0x91] = "Chunsoft",
            [0x92] = "Video system",
            [0x93] = "Ocean/Acclaim",
            [0x95] = "Varie",
            [0x96] = "Yonezawa/s’pal",
            [0x97] = "Kaneko",
            [0x99] = "Pack in soft",
            [0xA4] = "Konami (Yu-Gi-Oh!)",
        },
    .get_license = get_license,
    .get_rom_type = get_rom_type,
    .load = load,
    .read = read,
    .write = write,
    .mbc_1 = mbc_1,
    .battery = battery,
    .load_battery = load_battery,
    .save_battery = save_battery,
    .setup_banks = setup_banks,
};

const class_t *Cartridge = (const class_t *) &init_cartridge;
