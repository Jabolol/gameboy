#include "../include/gameboy.h"

static void constructor(void *ptr, va_list __attribute__((unused)) * args)
{
    CartridgeClass *self = (CartridgeClass *) ptr;
    if (!((self->context = calloc(1, sizeof(*self->context))))) {
        HANDLE_ERROR("failed memory allocation");
    }
}

static void destructor(void *ptr)
{
    CartridgeClass *self = (CartridgeClass *) ptr;
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
        HANDLE_ERROR("failed memory allocation");
    }
    fread(self->context->rom_data, self->context->rom_size, 1, stream);

    self->context->header = (rom_header_t *) (self->context->rom_data + 0x100);
    self->context->header->title[15] = 0;
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

    uint16_t x = 0;
    for (uint16_t i = 0x0134; i <= 0x014c; i++) {
        x -= self->context->rom_data[i] - 1;
    }

    printf("\tChecksum : %2.2X (%s)\n", self->context->header->checksum,
        (x & 0xFF) ? "PASSED" : "FAILED");

    return true;
}

static uint8_t read(CartridgeClass *self, uint16_t address)
{
    return self->context->rom_data[address];
}

static void write(CartridgeClass __attribute__((unused)) * self,
    uint16_t __attribute__((unused)) address,
    uint8_t __attribute__((unused)) value)
{
    HANDLE_ERROR("not implemented");
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
    .context = NULL,
    .get_license = get_license,
    .get_rom_type = get_rom_type,
    .load = load,
    .read = read,
    .write = write,
};

const class_t *Cartridge = (const class_t *) &init_cartridge;
