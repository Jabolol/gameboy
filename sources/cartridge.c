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

static bool load(CartridgeClass *self, const char *path)
{
    strncpy(self->context->filename, path, sizeof(self->context->filename));
    FILE *stream = fopen(path, "r");
    if (!stream) {
        fprintf(stderr, "Failed to open ROM (%s)\n", path);
        return false;
    }
    char opened_msg[256];
    snprintf(opened_msg, sizeof(opened_msg), "Opened: %s", path);
    LOG(opened_msg);
    fseek(stream, 0, SEEK_END);
    self->context->rom_size = ftell(stream);
    rewind(stream);

    if (!((self->context->rom_data = calloc(self->context->rom_size, 1)))) {
        fclose(stream);
        HANDLE_ERROR("failed memory allocation");
    }
    fread(self->context->rom_data, self->context->rom_size, 1, stream);

    self->context->header = (rom_header_t *) (self->context->rom_data + 0x100);
    self->context->header->title[14] = 0;
    self->context->has_battery = self->battery(self);
    self->context->has_rtc = self->rtc(self);
    self->context->needs_save = false;
    fclose(stream);

    char cart_msg[512];

    LOG("Cartridge loaded");

    snprintf(
        cart_msg, sizeof(cart_msg), "Title: %s", self->context->header->title);
    LOG(cart_msg);

    snprintf(cart_msg, sizeof(cart_msg), "Type: %2.2X (%s)",
        self->context->header->type, self->get_rom_type(self));
    LOG(cart_msg);

    snprintf(cart_msg, sizeof(cart_msg), "ROM Size: %d KB",
        32 << self->context->header->rom_size);
    LOG(cart_msg);

    snprintf(cart_msg, sizeof(cart_msg), "RAM Size: %2.2X KB",
        self->context->header->ram_size);
    LOG(cart_msg);

    snprintf(cart_msg, sizeof(cart_msg), "LIC Code: %2.2X (%s)",
        self->context->header->license_code, self->get_license(self));
    LOG(cart_msg);

    snprintf(cart_msg, sizeof(cart_msg), "ROM Vers: %2.2X",
        self->context->header->version);
    LOG(cart_msg);

    self->setup_banks(self);

    uint16_t x = 0;
    for (uint16_t i = 0x0134; i <= 0x014c; i++) {
        x -= self->context->rom_data[i] - 1;
    }

    snprintf(cart_msg, sizeof(cart_msg), "Checksum: %2.2X (%s)",
        self->context->header->checksum, (x & 0xFF) ? "PASSED" : "FAILED");
    LOG(cart_msg);

    if (self->context->has_battery) {
        self->load_battery(self);
    }

    return true;
}

static uint8_t read(CartridgeClass *self, uint16_t address)
{
    if (address < 0x4000) {
        if (self->mbc_1(self) && self->context->banking_mode == 1) {
            uint8_t rom_bank_mask = (self->context->rom_size / 0x4000) - 1;
            uint8_t bank =
                (self->context->rom_bank_value & 0xE0) & rom_bank_mask;
            return self->context->rom_data[(bank * 0x4000) + address];
        }
        return self->context->rom_data[address];
    }

    if (address < 0x8000) {
        if (self->context->rom_bank_x) {
            return self->context->rom_bank_x[address - 0x4000];
        }
        return self->context->rom_data[address];
    }

    if ((address & 0xE000) == 0xA000) {
        if (!self->context->ram_enabled) {
            return 0xFF;
        }

        if (self->context->ram_bank == NULL) {
            return 0xFF;
        }

        if (self->mbc_3(self) && self->context->rtc_selected) {
            return self->read_rtc(self, self->context->rtc_reg);
        }

        if (self->mbc_2(self)) {
            return self->context->ram_bank[(address - 0xA000) & 0x1FF] | 0xF0;
        }

        return self->context->ram_bank[address - 0xA000];
    }

    return 0xFF;
}

static void write(CartridgeClass *self, uint16_t address, uint8_t value)
{
    if (address < 0x8000) {
        if (self->mbc_1(self)) {
            self->write_mbc1(self, address, value);
        } else if (self->mbc_2(self)) {
            self->write_mbc2(self, address, value);
        } else if (self->mbc_3(self)) {
            self->write_mbc3(self, address, value);
        } else if (self->mbc_5(self)) {
            self->write_mbc5(self, address, value);
        } else if (self->mbc_6(self)) {
            self->write_mbc6(self, address, value);
        } else if (self->mbc_7(self)) {
            self->write_mbc7(self, address, value);
        }
        return;
    }

    if ((address & 0xE000) == 0xA000) {
        if (!self->context->ram_enabled) {
            return;
        }

        if (self->context->ram_bank == NULL) {
            return;
        }

        if (self->mbc_3(self) && self->context->rtc_selected) {
            self->write_rtc(self, self->context->rtc_reg, value);
            return;
        }

        if (self->mbc_2(self)) {
            self->context->ram_bank[(address - 0xA000) & 0x1FF] = value & 0x0F;
        } else {
            self->context->ram_bank[address - 0xA000] = value;
        }
        if (self->context->has_battery) {
            self->context->needs_save = true;
        }
    }
}

static void write_mbc1(CartridgeClass *self, uint16_t address, uint8_t value)
{
    switch (address & 0xE000) {
        case 0x0000: {
            self->context->ram_enabled = (value & 0x0F) == 0x0A;
            break;
        }
        case 0x2000: {
            uint8_t lower_bits = value & 0x1F;
            if (lower_bits == 0) {
                lower_bits = 1;
            }
            self->context->rom_bank_value =
                (self->context->rom_bank_value & 0x60) | lower_bits;
            uint8_t rom_bank_mask = (self->context->rom_size / 0x4000) - 1;
            uint8_t masked_bank =
                self->context->rom_bank_value & rom_bank_mask;
            self->context->rom_bank_x =
                self->context->rom_data + (0x4000 * masked_bank);
            break;
        }
        case 0x4000: {
            uint8_t upper_bits = (value & 0x03) << 5;
            self->context->rom_bank_value =
                (self->context->rom_bank_value & 0x1F) | upper_bits;
            uint8_t rom_bank_mask = (self->context->rom_size / 0x4000) - 1;
            uint8_t masked_bank =
                self->context->rom_bank_value & rom_bank_mask;
            self->context->rom_bank_x =
                self->context->rom_data + (0x4000 * masked_bank);
            self->context->ram_bank_value = value & 0x03;
            if (self->context->ram_banking) {
                if (self->context->needs_save) {
                    self->save_battery(self);
                }
                if (self->context->ram_banks[self->context->ram_bank_value]
                    != NULL) {
                    self->context->ram_bank =
                        self->context
                            ->ram_banks[self->context->ram_bank_value];
                }
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
                if (self->context->ram_banks[self->context->ram_bank_value]
                    != NULL) {
                    self->context->ram_bank =
                        self->context
                            ->ram_banks[self->context->ram_bank_value];
                }
            }
            break;
        }
    }
}

static void write_mbc2(CartridgeClass *self, uint16_t address, uint8_t value)
{
    switch (address & 0xE000) {
        case 0x0000: {
            if (!(address & 0x0100)) {
                self->context->ram_enabled = (value & 0x0F) == 0x0A;
            }
            break;
        }
        case 0x2000: {
            if (address & 0x0100) {
                value &= 0x0F;
                if (value == 0) {
                    value = 1;
                }
                self->context->rom_bank_value = value;
                self->context->rom_bank_x = self->context->rom_data
                    + (0x4000 * self->context->rom_bank_value);
            }
            break;
        }
    }
}

static void write_mbc3(CartridgeClass *self, uint16_t address, uint8_t value)
{
    switch (address & 0xE000) {
        case 0x0000: {
            self->context->ram_enabled = (value & 0x0F) == 0x0A;
            break;
        }
        case 0x2000: {
            value &= 0x7F;
            if (value == 0) {
                value = 1;
            }
            self->context->rom_bank_value = value;
            self->context->rom_bank_x = self->context->rom_data
                + (0x4000 * self->context->rom_bank_value);
            break;
        }
        case 0x4000: {
            if (value <= 0x03) {
                self->context->ram_bank_value = value;
                self->context->rtc_selected = false;
                if (self->context->ram_banks[value] != NULL) {
                    self->context->ram_bank = self->context->ram_banks[value];
                }
            } else if (value >= 0x08 && value <= 0x0C) {
                self->context->rtc_selected = true;
                self->context->rtc_reg = value - 0x08;
            }
            break;
        }
        case 0x6000: {
            if (self->context->rtc_latch == 0 && value == 1) {
                self->latch_rtc(self);
            }
            self->context->rtc_latch = value;
            break;
        }
    }
}

static void write_mbc5(CartridgeClass *self, uint16_t address, uint8_t value)
{
    switch (address & 0xF000) {
        case 0x0000:
        case 0x1000: {
            self->context->ram_enabled = (value & 0x0F) == 0x0A;
            break;
        }
        case 0x2000: {
            self->context->rom_bank_value =
                (self->context->rom_bank_value & 0x100) | value;
            self->context->rom_bank_x = self->context->rom_data
                + (0x4000 * self->context->rom_bank_value);
            break;
        }
        case 0x3000: {
            self->context->rom_bank_value =
                (self->context->rom_bank_value & 0xFF) | ((value & 0x01) << 8);
            self->context->rom_bank_x = self->context->rom_data
                + (0x4000 * self->context->rom_bank_value);
            break;
        }
        case 0x4000:
        case 0x5000: {
            value &= 0x0F;
            self->context->ram_bank_value = value;

            if (self->context->ram_banks[value] != NULL) {
                self->context->ram_bank = self->context->ram_banks[value];
            }
            break;
        }
    }
}

static void write_mbc6(CartridgeClass *self, uint16_t address, uint8_t value)
{
    switch (address & 0xF000) {
        case 0x0000:
        case 0x1000: {
            self->context->ram_enabled = (value & 0x0F) == 0x0A;
            break;
        }
        case 0x2000: {
            if (address & 0x0020) {
                self->context->mbc6_rom_bank1 =
                    (self->context->mbc6_rom_bank1 & 0x1F)
                    | ((value & 0x03) << 5);
            } else {
                self->context->mbc6_rom_bank1 =
                    (self->context->mbc6_rom_bank1 & 0x60) | (value & 0x1F);
            }

            self->update_mbc6_banks(self);
            break;
        }
        case 0x3000: {
            if (address & 0x0020) {
                self->context->mbc6_rom_bank2 =
                    (self->context->mbc6_rom_bank2 & 0x1F)
                    | ((value & 0x03) << 5);
            } else {
                self->context->mbc6_rom_bank2 =
                    (self->context->mbc6_rom_bank2 & 0x60) | (value & 0x1F);
            }

            self->update_mbc6_banks(self);
            break;
        }
        case 0x4000: {
            self->context->ram_bank_value = value & 0x07;
            if (self->context->ram_banks[self->context->ram_bank_value]
                != NULL) {
                self->context->ram_bank =
                    self->context->ram_banks[self->context->ram_bank_value];
            }
            break;
        }
    }
}

static void write_mbc7(CartridgeClass *self, uint16_t address, uint8_t value)
{
    switch (address & 0xF000) {
        case 0x0000:
        case 0x1000: {
            self->context->ram_enabled = (value & 0x0F) == 0x0A;
            break;
        }
        case 0x2000:
        case 0x3000: {
            value &= 0x7F;
            if (value == 0) {
                value = 1;
            }
            self->context->rom_bank_value = value;
            self->context->rom_bank_x = self->context->rom_data
                + (0x4000 * self->context->rom_bank_value);
            break;
        }
        case 0xA000: {
            if (address == 0xA000) {
                self->context->mbc7_cs = (value & 0x80) != 0;
                self->context->mbc7_clk = (value & 0x40) != 0;

                if (self->context->mbc7_cs && self->context->ram_enabled) {
                    if (self->context->mbc7_prev_clk
                        && !self->context->mbc7_clk) {
                        self->handle_mbc7_transfer(self, value);
                    }
                    self->context->mbc7_prev_clk = self->context->mbc7_clk;
                }
            }
            break;
        }
    }
}

static void update_mbc6_banks(CartridgeClass *self)
{
    if (self->context->mbc6_rom_bank1 == 0) {
        self->context->mbc6_rom_bank1 = 1;
    }
    if (self->context->mbc6_rom_bank2 == 0) {
        self->context->mbc6_rom_bank2 = 1;
    }

    self->context->rom_bank_value = self->context->mbc6_rom_bank1;
    self->context->rom_bank_x =
        self->context->rom_data + (0x4000 * self->context->rom_bank_value);
}

static void handle_mbc7_transfer(CartridgeClass *self, uint8_t value)
{
    if (self->context->mbc7_state == 0) {
        self->context->mbc7_buffer = value & 0x3F;
        self->context->mbc7_state = 1;
        return;
    }

    switch (self->context->mbc7_buffer) {
        case 0x08: {
            self->context->mbc7_output = 0x8000;
            break;
        }
        case 0x09: {
            self->context->mbc7_output = 0x8000;
            break;
        }
        case 0x0B: {
            self->context->mbc7_output = 0xFF;
            break;
        }
        case 0x0C: {
            if (self->context->has_battery) {
                self->context->needs_save = true;
            }
            break;
        }
    }
}

static uint8_t read_rtc(CartridgeClass *self, uint8_t reg)
{
    switch (reg) {
        case 0: return self->context->rtc_s;
        case 1: return self->context->rtc_m;
        case 2: return self->context->rtc_h;
        case 3: return self->context->rtc_dl;
        case 4: return self->context->rtc_dh;
        default: return 0xFF;
    }
}

static void write_rtc(CartridgeClass *self, uint8_t reg, uint8_t value)
{
    switch (reg) {
        case 0: self->context->rtc_s = value; break;
        case 1: self->context->rtc_m = value; break;
        case 2: self->context->rtc_h = value; break;
        case 3: self->context->rtc_dl = value; break;
        case 4: self->context->rtc_dh = value & 0xC1; break;
    }
}

static void latch_rtc(CartridgeClass *self)
{
    time_t current_time;
    time(&current_time);

    time_t seconds_elapsed = current_time - self->context->rtc_last_time;
    self->context->rtc_last_time = current_time;

    if (!(self->context->rtc_dh & 0x40)) {
        uint32_t seconds = self->context->rtc_s;
        uint32_t minutes = self->context->rtc_m;
        uint32_t hours = self->context->rtc_h;
        uint32_t days =
            self->context->rtc_dl | ((self->context->rtc_dh & 0x01) << 8);
        bool carry = (self->context->rtc_dh & 0x80) != 0;

        seconds += seconds_elapsed % 60;
        seconds_elapsed /= 60;

        if (seconds >= 60) {
            seconds -= 60;
            minutes++;
        }

        minutes += seconds_elapsed % 60;
        seconds_elapsed /= 60;

        if (minutes >= 60) {
            minutes -= 60;
            hours++;
        }

        hours += seconds_elapsed % 24;
        seconds_elapsed /= 24;

        if (hours >= 24) {
            hours -= 24;
            days++;
        }

        days += seconds_elapsed;

        if (days > 511) {
            carry = true;
            days %= 512;
        }

        self->context->rtc_s = seconds;
        self->context->rtc_m = minutes;
        self->context->rtc_h = hours;
        self->context->rtc_dl = days & 0xFF;
        self->context->rtc_dh = (self->context->rtc_dh & 0x7E)
            | ((days >> 8) & 0x01) | (carry ? 0x80 : 0);
    }
}

static bool mbc_1(CartridgeClass *self)
{
    uint8_t type = self->context->header->type;
    return type >= 0x01 && type <= 0x03;
}

static bool mbc_2(CartridgeClass *self)
{
    uint8_t type = self->context->header->type;
    return type == 0x05 || type == 0x06;
}

static bool mbc_3(CartridgeClass *self)
{
    uint8_t type = self->context->header->type;
    return (type >= 0x0F && type <= 0x13);
}

static bool mbc_5(CartridgeClass *self)
{
    uint8_t type = self->context->header->type;
    return (type >= 0x19 && type <= 0x1E);
}

static bool mbc_6(CartridgeClass *self)
{
    uint8_t type = self->context->header->type;
    return type == 0x20;
}

static bool mbc_7(CartridgeClass *self)
{
    uint8_t type = self->context->header->type;
    return type == 0x22;
}

static bool battery(CartridgeClass *self)
{
    uint8_t type = self->context->header->type;
    return type == 0x03 || type == 0x06 || type == 0x09 || type == 0x0D
        || type == 0x0F || type == 0x10 || type == 0x13 || type == 0x1B
        || type == 0x1E || type == 0x22;
}

static bool rtc(CartridgeClass *self)
{
    uint8_t type = self->context->header->type;
    return type == 0x0F || type == 0x10;
}

static void setup_banks(CartridgeClass *self)
{
    self->set_banks = 0;

    for (int i = 0; i < 16; i++) {
        self->context->ram_banks[i] = NULL;

        if (self->mbc_2(self) && i == 0) {
            self->context->ram_banks[i] = calloc(512, 1);
            self->set_banks |= 1 << i;
        } else if (self->context->header->ram_size == 2 && i == 0) {
            self->context->ram_banks[i] = calloc(0x2000, 1);
            self->set_banks |= 1 << i;
        } else if (self->context->header->ram_size == 3 && i < 4) {
            self->context->ram_banks[i] = calloc(0x2000, 1);
            self->set_banks |= 1 << i;
        } else if (self->context->header->ram_size == 4 && i < 16) {
            self->context->ram_banks[i] = calloc(0x2000, 1);
            self->set_banks |= 1 << i;
        } else if (self->context->header->ram_size == 5 && i < 8) {
            self->context->ram_banks[i] = calloc(0x2000, 1);
            self->set_banks |= 1 << i;
        }
    }

    self->context->ram_bank = self->context->ram_banks[0];
    self->context->rom_bank_x = self->context->rom_data + 0x4000;

    if (self->mbc_3(self) && self->context->has_rtc) {
        self->context->rtc_s = 0;
        self->context->rtc_m = 0;
        self->context->rtc_h = 0;
        self->context->rtc_dl = 0;
        self->context->rtc_dh = 0;
        time(&self->context->rtc_last_time);
    }

    if (self->mbc_7(self)) {
        self->context->mbc7_state = 0;
        self->context->mbc7_buffer = 0;
        self->context->mbc7_cs = false;
        self->context->mbc7_clk = false;
        self->context->mbc7_prev_clk = false;
    }
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

    if (self->mbc_2(self)) {
        fread(self->context->ram_bank, 512, 1, stream);
    } else {
        for (int i = 0; i < 16; i++) {
            if (self->set_banks & (1 << i) && self->context->ram_banks[i]) {
                fread(self->context->ram_banks[i], 0x2000, 1, stream);
            }
        }
    }

    if (self->mbc_3(self) && self->context->has_rtc) {
        fread(&self->context->rtc_s, sizeof(uint8_t), 1, stream);
        fread(&self->context->rtc_m, sizeof(uint8_t), 1, stream);
        fread(&self->context->rtc_h, sizeof(uint8_t), 1, stream);
        fread(&self->context->rtc_dl, sizeof(uint8_t), 1, stream);
        fread(&self->context->rtc_dh, sizeof(uint8_t), 1, stream);
        fread(&self->context->rtc_last_time, sizeof(time_t), 1, stream);

        self->latch_rtc(self);
    }

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

    if (self->mbc_2(self)) {
        fwrite(self->context->ram_bank, 512, 1, stream);
    } else {
        for (int i = 0; i < 16; i++) {
            if (self->set_banks & (1 << i) && self->context->ram_banks[i]) {
                fwrite(self->context->ram_banks[i], 0x2000, 1, stream);
            }
        }
    }

    if (self->mbc_3(self) && self->context->has_rtc) {
        self->latch_rtc(self);

        fwrite(&self->context->rtc_s, sizeof(uint8_t), 1, stream);
        fwrite(&self->context->rtc_m, sizeof(uint8_t), 1, stream);
        fwrite(&self->context->rtc_h, sizeof(uint8_t), 1, stream);
        fwrite(&self->context->rtc_dl, sizeof(uint8_t), 1, stream);
        fwrite(&self->context->rtc_dh, sizeof(uint8_t), 1, stream);
        fwrite(&self->context->rtc_last_time, sizeof(time_t), 1, stream);
    }

    fclose(stream);
    self->context->needs_save = false;
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
            [0x96] = "Yonezawa/s'pal",
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
    .mbc_2 = mbc_2,
    .mbc_3 = mbc_3,
    .mbc_5 = mbc_5,
    .mbc_6 = mbc_6,
    .mbc_7 = mbc_7,
    .battery = battery,
    .rtc = rtc,
    .write_mbc1 = write_mbc1,
    .write_mbc2 = write_mbc2,
    .write_mbc3 = write_mbc3,
    .write_mbc5 = write_mbc5,
    .write_mbc6 = write_mbc6,
    .write_mbc7 = write_mbc7,
    .update_mbc6_banks = update_mbc6_banks,
    .handle_mbc7_transfer = handle_mbc7_transfer,
    .read_rtc = read_rtc,
    .write_rtc = write_rtc,
    .latch_rtc = latch_rtc,
    .setup_banks = setup_banks,
    .load_battery = load_battery,
    .save_battery = save_battery,
};

const class_t *Cartridge = (const class_t *) &init_cartridge;
