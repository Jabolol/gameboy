#include "../include/gameboy.h"

#define RTC_SAVE_SIZE    48
#define LEGACY_RTC_SIZE  13
#define HUC3_SAVE_SIZE   16
#define MBC6_FLASH_SIZE  0x100000
#define MBC7_EEPROM_SIZE 256

static uint8_t flash_status_page[0x2000];
static uint8_t flash_id_page[0x2000];

static void constructor(void *ptr, va_list *args)
{
    CartridgeClass *self = (CartridgeClass *) ptr;
    if (!((self->context = calloc(1, sizeof(*self->context))))) {
        HANDLE_ERROR("failed memory allocation");
    }
    self->parent = va_arg(*args, GameboyClass *);
}

static void destructor(void *ptr)
{
    CartridgeClass *self = (CartridgeClass *) ptr;

    if (self->context->needs_save) {
        self->save_battery(self);
    }
    free(self->context->mbc6.flash);
    free(self->context->ram_data);
    free(self->context->rom_data);
    free(self->context);
}

static const char *get_license(CartridgeClass *self)
{
    rom_header_t *header = self->context->header;

    if (header->license_code == 0x33) {
        return "NEW LICENSEE CODE";
    }
    if (header->license_code < 0xA5
        && self->license_codes[header->license_code]) {
        return self->license_codes[header->license_code];
    }
    return "UNKNOWN";
}

static const char *get_rom_type(CartridgeClass *self)
{
    const char *name = self->types[self->context->header->type].name;
    return name ? name : "UNKNOWN";
}

static void map_rom(CartridgeClass *self, uint8_t slot, uint32_t bank)
{
    cartridge_context_t *ctx = self->context;
    uint8_t *base = ctx->rom_data + ((bank & (ctx->rom_banks - 1)) * 0x4000);

    ctx->rom_map[slot * 2] = base;
    ctx->rom_map[slot * 2 + 1] = base + 0x2000;
}

static void unmap_ram(CartridgeClass *self)
{
    for (uint8_t i = 0; i < 2; i++) {
        self->context->ram_read_map[i] = NULL;
        self->context->ram_write_map[i] = NULL;
    }
}

static void map_ram(
    CartridgeClass *self, uint32_t bank, bool readable, bool writable)
{
    cartridge_context_t *ctx = self->context;

    self->unmap_ram(self);
    if (!ctx->ram_banks) {
        return;
    }

    uint8_t *base = ctx->ram_data + ((bank & (ctx->ram_banks - 1)) * 0x2000);
    for (uint8_t i = 0; i < 2; i++) {
        ctx->ram_read_map[i] = readable ? base + (i * 0x1000) : NULL;
        ctx->ram_write_map[i] = writable ? base + (i * 0x1000) : NULL;
    }
}

static uint8_t read(CartridgeClass *self, uint16_t address)
{
    if (address < 0x8000) {
        return self->context->rom_map[address >> 13][address & 0x1FFF];
    }

    uint8_t *ram = self->context->ram_read_map[(address >> 12) & 1];
    if (ram) {
        return ram[address & 0x0FFF];
    }
    return self->ram_read(self, address);
}

static void write(CartridgeClass *self, uint16_t address, uint8_t value)
{
    if (address < 0x8000) {
        self->mbc_write(self, address, value);
        return;
    }

    uint8_t *ram = self->context->ram_write_map[(address >> 12) & 1];
    if (ram) {
        ram[address & 0x0FFF] = value;
        self->context->needs_save |= self->context->has_battery;
        return;
    }
    self->ram_write(self, address, value);
}

static uint8_t read_open_bus(
    CartridgeClass UNUSED *self, uint16_t UNUSED address)
{
    return 0xFF;
}

static void write_ignored(
    CartridgeClass UNUSED *self, uint16_t UNUSED address, uint8_t UNUSED value)
{
}

static bool ram_enable_value(uint8_t value)
{
    return (value & 0x0F) == 0x0A;
}

static void update_mbc1(CartridgeClass *self)
{
    cartridge_context_t *ctx = self->context;
    uint8_t shift = ctx->mbc == MBC_1M ? 4 : 5;
    uint8_t low_mask = ctx->mbc == MBC_1M ? 0x0F : 0x1F;
    uint32_t high = ctx->ram_bank_value << shift;

    self->map_rom(self, 0, ctx->banking_mode ? high : 0);
    self->map_rom(self, 1, high | (ctx->rom_bank_value & low_mask));
    self->map_ram(self, ctx->banking_mode ? ctx->ram_bank_value : 0,
        ctx->ram_enabled, ctx->ram_enabled);
}

static void write_mbc1(CartridgeClass *self, uint16_t address, uint8_t value)
{
    cartridge_context_t *ctx = self->context;

    switch (address & 0x6000) {
        case 0x0000: ctx->ram_enabled = ram_enable_value(value); break;
        case 0x2000: {
            ctx->rom_bank_value = (value & 0x1F) ? (value & 0x1F) : 1;
            break;
        }
        case 0x4000: ctx->ram_bank_value = value & 0x03; break;
        case 0x6000: ctx->banking_mode = value & 0x01; break;
    }
    self->update_mbc1(self);
}

static void write_mbc2(CartridgeClass *self, uint16_t address, uint8_t value)
{
    if (address >= 0x4000) {
        return;
    }
    if (address & 0x0100) {
        self->context->rom_bank_value = (value & 0x0F) ? (value & 0x0F) : 1;
        self->update_mapping(self);
    } else {
        self->context->ram_enabled = ram_enable_value(value);
    }
}

static uint8_t read_mbc2_ram(CartridgeClass *self, uint16_t address)
{
    if (!self->context->ram_enabled) {
        return 0xFF;
    }
    return self->context->ram_data[address & 0x01FF] | 0xF0;
}

static void write_mbc2_ram(
    CartridgeClass *self, uint16_t address, uint8_t value)
{
    if (!self->context->ram_enabled) {
        return;
    }
    self->context->ram_data[address & 0x01FF] = value & 0x0F;
    self->context->needs_save |= self->context->has_battery;
}

static void rtc_step(mbc3_rtc_t *rtc)
{
    rtc->s = (rtc->s + 1) & 0x3F;
    if (rtc->s != 60) {
        return;
    }
    rtc->s = 0;
    rtc->m = (rtc->m + 1) & 0x3F;
    if (rtc->m != 60) {
        return;
    }
    rtc->m = 0;
    rtc->h = (rtc->h + 1) & 0x1F;
    if (rtc->h != 24) {
        return;
    }
    rtc->h = 0;
    rtc->d = (rtc->d + 1) & 0x1FF;
    if (rtc->d == 0) {
        rtc->carry = true;
    }
}

static void rtc_advance(CartridgeClass *self, uint64_t seconds)
{
    mbc3_rtc_t *rtc = &self->context->rtc;

    if (rtc->halt) {
        return;
    }
    while (seconds > 0 && (rtc->s >= 60 || rtc->m >= 60 || rtc->h >= 24)) {
        rtc_step(rtc);
        seconds--;
    }
    if (seconds == 0) {
        return;
    }

    uint64_t total = rtc->s + 60 * (rtc->m + 60 * (rtc->h + 24 * rtc->d));
    total += seconds;

    uint64_t days = total / 86400;
    rtc->s = total % 60;
    rtc->m = (total / 60) % 60;
    rtc->h = (total / 3600) % 24;
    if (days > 0x1FF) {
        rtc->carry = true;
    }
    rtc->d = days & 0x1FF;
}

static void rtc_update(CartridgeClass *self)
{
    mbc3_rtc_t *rtc = &self->context->rtc;
    uint64_t now = self->parent->context->ticks;
    uint64_t elapsed = now - rtc->last_tick;

    rtc->last_tick = now;
    if (rtc->halt) {
        return;
    }

    uint64_t total = rtc->sub_second + elapsed;
    rtc->sub_second = total % CLOCK_SPEED;
    self->rtc_advance(self, total / CLOCK_SPEED);
}

static void latch_rtc(CartridgeClass *self)
{
    mbc3_rtc_t *rtc = &self->context->rtc;

    self->rtc_update(self);
    rtc->latched[0] = rtc->s;
    rtc->latched[1] = rtc->m;
    rtc->latched[2] = rtc->h;
    rtc->latched[3] = rtc->d & 0xFF;
    rtc->latched[4] = ((rtc->d >> 8) & 0x01) | (rtc->halt ? 0x40 : 0x00)
        | (rtc->carry ? 0x80 : 0x00);
}

static uint8_t read_rtc(CartridgeClass *self, uint8_t reg)
{
    return reg < 5 ? self->context->rtc.latched[reg] : 0xFF;
}

static void write_rtc(CartridgeClass *self, uint8_t reg, uint8_t value)
{
    mbc3_rtc_t *rtc = &self->context->rtc;

    self->rtc_update(self);
    switch (reg) {
        case 0: {
            rtc->s = value & 0x3F;
            rtc->sub_second = 0;
            break;
        }
        case 1: rtc->m = value & 0x3F; break;
        case 2: rtc->h = value & 0x1F; break;
        case 3: rtc->d = (rtc->d & 0x100) | value; break;
        case 4: {
            rtc->d = (rtc->d & 0xFF) | ((value & 0x01) << 8);
            rtc->halt = value & 0x40;
            rtc->carry = value & 0x80;
            break;
        }
        default: return;
    }

    static const uint8_t masks[5] = {0x3F, 0x3F, 0x1F, 0xFF, 0xC1};
    rtc->latched[reg] = value & masks[reg];
    self->context->needs_save |= self->context->has_battery;
}

static void update_banks(CartridgeClass *self)
{
    cartridge_context_t *ctx = self->context;
    bool enabled = ctx->ram_enabled && !ctx->rtc_selected;

    self->map_rom(self, 1, ctx->rom_bank_value);
    self->map_ram(self, ctx->ram_bank_value, enabled, enabled);
}

static void write_mbc3(CartridgeClass *self, uint16_t address, uint8_t value)
{
    cartridge_context_t *ctx = self->context;

    switch (address & 0x6000) {
        case 0x0000: ctx->ram_enabled = ram_enable_value(value); break;
        case 0x2000: {
            value &= ctx->rom_banks > 128 ? 0xFF : 0x7F;
            ctx->rom_bank_value = value ? value : 1;
            break;
        }
        case 0x4000: {
            if (value <= 0x07) {
                ctx->ram_bank_value = value;
                ctx->rtc_selected = false;
            } else if (ctx->has_rtc && value >= 0x08 && value <= 0x0C) {
                ctx->rtc_selected = true;
                ctx->rtc_reg = value - 0x08;
            }
            break;
        }
        case 0x6000: {
            if (ctx->has_rtc) {
                if (ctx->rtc.latch_state == 0 && value == 1) {
                    self->latch_rtc(self);
                }
                ctx->rtc.latch_state = value;
            }
            break;
        }
    }
    self->update_banks(self);
}

static uint8_t read_mbc3_ram(CartridgeClass *self, uint16_t UNUSED address)
{
    if (self->context->ram_enabled && self->context->rtc_selected) {
        return self->read_rtc(self, self->context->rtc_reg);
    }
    return 0xFF;
}

static void write_mbc3_ram(
    CartridgeClass *self, uint16_t UNUSED address, uint8_t value)
{
    if (self->context->ram_enabled && self->context->rtc_selected) {
        self->write_rtc(self, self->context->rtc_reg, value);
    }
}

static void write_mbc5(CartridgeClass *self, uint16_t address, uint8_t value)
{
    cartridge_context_t *ctx = self->context;

    switch (address & 0x7000) {
        case 0x0000:
        case 0x1000: ctx->ram_enabled = ram_enable_value(value); break;
        case 0x2000: {
            ctx->rom_bank_value = (ctx->rom_bank_value & 0x100) | value;
            break;
        }
        case 0x3000: {
            ctx->rom_bank_value =
                (ctx->rom_bank_value & 0xFF) | ((value & 0x01) << 8);
            break;
        }
        case 0x4000:
        case 0x5000: {
            if (ctx->has_rumble) {
                ctx->rumble = value & 0x08;
                ctx->ram_bank_value = value & 0x07;
            } else {
                ctx->ram_bank_value = value & 0x0F;
            }
            break;
        }
        default: return;
    }
    self->update_banks(self);
}

static void update_mbc6(CartridgeClass *self)
{
    cartridge_context_t *ctx = self->context;
    mbc6_state_t *mbc6 = &ctx->mbc6;
    uint32_t rom_banks8 = ctx->rom_banks * 2;

    for (uint8_t i = 0; i < 2; i++) {
        if (mbc6->rom_flash[i] && mbc6->flash_enabled) {
            if (mbc6->flash_mode == FLASH_ID) {
                ctx->rom_map[2 + i] = flash_id_page;
            } else if (mbc6->flash_mode != FLASH_READ) {
                ctx->rom_map[2 + i] = flash_status_page;
            } else {
                ctx->rom_map[2 + i] =
                    mbc6->flash + ((mbc6->rom_bank[i] & 0x7F) * 0x2000);
            }
        } else {
            ctx->rom_map[2 + i] = ctx->rom_data
                + ((mbc6->rom_bank[i] & (rom_banks8 - 1)) * 0x2000);
        }

        uint8_t *ram = ctx->ram_enabled
            ? ctx->ram_data + ((mbc6->ram_bank[i] & 0x07) * 0x1000)
            : NULL;
        ctx->ram_read_map[i] = ram;
        ctx->ram_write_map[i] = ram;
    }
}

static void flash_command(
    CartridgeClass *self, uint8_t window, uint16_t address, uint8_t value)
{
    mbc6_state_t *mbc6 = &self->context->mbc6;
    uint32_t flash_address =
        ((mbc6->rom_bank[window] & 0x7F) * 0x2000) | (address & 0x1FFF);
    uint16_t command_address = flash_address & 0x7FFF;
    bool sector0 = flash_address < 0x20000;

    if (value == 0xF0) {
        mbc6->flash_state = 0;
        mbc6->flash_mode = FLASH_READ;
        return;
    }
    if (mbc6->flash_mode == FLASH_PROGRAM) {
        if (!sector0 || mbc6->flash_write_enabled) {
            mbc6->flash[flash_address] &= value;
            self->context->needs_save = true;
        }
        return;
    }

    switch (mbc6->flash_state) {
        case 0:
        case 3: {
            if (command_address == 0x5555 && value == 0xAA) {
                mbc6->flash_state += 1;
            }
            return;
        }
        case 1:
        case 4: {
            mbc6->flash_state = (command_address == 0x2AAA && value == 0x55)
                ? mbc6->flash_state + 1
                : 0;
            return;
        }
        case 2: {
            mbc6->flash_state = 0;
            switch (value) {
                case 0x80: mbc6->flash_state = 3; break;
                case 0x90: mbc6->flash_mode = FLASH_ID; break;
                case 0xA0: mbc6->flash_mode = FLASH_PROGRAM; break;
            }
            return;
        }
        case 5: {
            mbc6->flash_state = 0;
            if (value == 0x30) {
                uint32_t sector = flash_address & ~0x1FFFF;
                if (sector != 0 || mbc6->flash_write_enabled) {
                    memset(mbc6->flash + sector, 0xFF, 0x20000);
                }
            } else if (value == 0x10) {
                uint32_t start = mbc6->flash_write_enabled ? 0 : 0x20000;
                memset(mbc6->flash + start, 0xFF, MBC6_FLASH_SIZE - start);
            } else {
                return;
            }
            mbc6->flash_mode = FLASH_STATUS;
            self->context->needs_save = true;
            return;
        }
    }
}

static void write_mbc6(CartridgeClass *self, uint16_t address, uint8_t value)
{
    cartridge_context_t *ctx = self->context;
    mbc6_state_t *mbc6 = &ctx->mbc6;

    if (address >= 0x4000) {
        uint8_t window = (address >> 13) & 1;
        if (mbc6->rom_flash[window] && mbc6->flash_enabled) {
            self->flash_command(self, window, address, value);
        }
    } else {
        switch (address & 0x3C00) {
            case 0x0000: ctx->ram_enabled = ram_enable_value(value); break;
            case 0x0400: mbc6->ram_bank[0] = value & 0x07; break;
            case 0x0800: mbc6->ram_bank[1] = value & 0x07; break;
            case 0x0C00: mbc6->flash_enabled = value & 0x01; break;
            case 0x1000:
            case 0x1400:
            case 0x1800:
            case 0x1C00: mbc6->flash_write_enabled = value & 0x01; break;
            case 0x2000:
            case 0x2400: mbc6->rom_bank[0] = value & 0x7F; break;
            case 0x2800:
            case 0x2C00: mbc6->rom_flash[0] = value & 0x08; break;
            case 0x3000:
            case 0x3400: mbc6->rom_bank[1] = value & 0x7F; break;
            case 0x3800:
            case 0x3C00: mbc6->rom_flash[1] = value & 0x08; break;
        }
    }
    self->update_mbc6(self);
}

static void write_mbc7(CartridgeClass *self, uint16_t address, uint8_t value)
{
    cartridge_context_t *ctx = self->context;

    switch (address & 0x6000) {
        case 0x0000: ctx->ram_enabled = ram_enable_value(value); break;
        case 0x2000: {
            ctx->rom_bank_value = value & 0x7F;
            self->update_mapping(self);
            break;
        }
        case 0x4000: ctx->mbc7.ram_enabled2 = value == 0x40; break;
    }
}

static uint16_t tilt(JoypadClass *joypad, button_t positive, button_t negative)
{
    return 0x81D0 - (joypad->pressed(joypad, positive) ? 0x70 : 0)
        + (joypad->pressed(joypad, negative) ? 0x70 : 0);
}

static void eeprom_command(CartridgeClass *self)
{
    mbc7_state_t *mbc7 = &self->context->mbc7;
    uint8_t *word = mbc7->eeprom + (mbc7->address * 2);

    mbc7->state = EEPROM_IDLE;
    mbc7->bits = 0;
    switch ((mbc7->shift >> 8) & 0x03) {
        case 0x00: {
            switch ((mbc7->shift >> 6) & 0x03) {
                case 0x00: mbc7->write_enabled = false; break;
                case 0x01: mbc7->state = EEPROM_WRITE_ALL; break;
                case 0x02: {
                    if (mbc7->write_enabled) {
                        memset(mbc7->eeprom, 0xFF, MBC7_EEPROM_SIZE);
                        self->context->needs_save = true;
                    }
                    mbc7->do_bit = true;
                    break;
                }
                case 0x03: mbc7->write_enabled = true; break;
            }
            break;
        }
        case 0x01: mbc7->state = EEPROM_WRITE; break;
        case 0x02: {
            mbc7->state = EEPROM_READ;
            mbc7->shift = (word[0] << 8) | word[1];
            mbc7->do_bit = false;
            return;
        }
        case 0x03: {
            if (mbc7->write_enabled) {
                word[0] = word[1] = 0xFF;
                self->context->needs_save = true;
            }
            mbc7->do_bit = true;
            break;
        }
    }
    mbc7->shift = 0;
}

static void eeprom_write(CartridgeClass *self, uint8_t value)
{
    mbc7_state_t *mbc7 = &self->context->mbc7;
    bool cs = value & 0x80;
    bool clk = value & 0x40;
    bool di = value & 0x02;

    if (!cs || !mbc7->cs) {
        mbc7->state = EEPROM_IDLE;
    }
    if (cs && !mbc7->clk && clk) {
        switch (mbc7->state) {
            case EEPROM_IDLE: {
                if (di) {
                    mbc7->state = EEPROM_COMMAND;
                    mbc7->shift = 0;
                    mbc7->bits = 0;
                }
                break;
            }
            case EEPROM_COMMAND: {
                mbc7->shift = (mbc7->shift << 1) | di;
                if (++mbc7->bits == 10) {
                    mbc7->address = mbc7->shift & 0x7F;
                    self->eeprom_command(self);
                }
                break;
            }
            case EEPROM_READ: {
                mbc7->do_bit = (mbc7->shift >> 15) & 1;
                mbc7->shift <<= 1;
                if (++mbc7->bits == 16) {
                    mbc7->state = EEPROM_IDLE;
                }
                break;
            }
            case EEPROM_WRITE:
            case EEPROM_WRITE_ALL: {
                mbc7->shift = (mbc7->shift << 1) | di;
                if (++mbc7->bits < 16) {
                    break;
                }
                if (mbc7->write_enabled) {
                    bool all = mbc7->state == EEPROM_WRITE_ALL;
                    for (uint8_t i = all ? 0 : mbc7->address;
                        i <= (all ? 127 : mbc7->address); i++) {
                        mbc7->eeprom[i * 2] = mbc7->shift >> 8;
                        mbc7->eeprom[i * 2 + 1] = mbc7->shift & 0xFF;
                    }
                    self->context->needs_save = true;
                }
                mbc7->do_bit = true;
                mbc7->state = EEPROM_IDLE;
                break;
            }
        }
    }
    mbc7->cs = cs;
    mbc7->clk = clk;
    mbc7->di = di;
}

static uint8_t read_mbc7_ram(CartridgeClass *self, uint16_t address)
{
    mbc7_state_t *mbc7 = &self->context->mbc7;

    if (!self->context->ram_enabled || !mbc7->ram_enabled2
        || address >= 0xB000) {
        return 0xFF;
    }
    switch ((address >> 4) & 0x0F) {
        case 0x2: return mbc7->accel_x & 0xFF;
        case 0x3: return mbc7->accel_x >> 8;
        case 0x4: return mbc7->accel_y & 0xFF;
        case 0x5: return mbc7->accel_y >> 8;
        case 0x6: return 0x00;
        case 0x8:
            return (mbc7->cs ? 0x80 : 0) | (mbc7->clk ? 0x40 : 0)
                | (mbc7->di ? 0x02 : 0) | (mbc7->do_bit ? 0x01 : 0);
        default: return 0xFF;
    }
}

static void write_mbc7_ram(
    CartridgeClass *self, uint16_t address, uint8_t value)
{
    mbc7_state_t *mbc7 = &self->context->mbc7;

    if (!self->context->ram_enabled || !mbc7->ram_enabled2
        || address >= 0xB000) {
        return;
    }
    switch ((address >> 4) & 0x0F) {
        case 0x0: {
            if (value == 0x55) {
                mbc7->latched = false;
                mbc7->accel_x = 0x8000;
                mbc7->accel_y = 0x8000;
            }
            break;
        }
        case 0x1: {
            if (value == 0xAA && !mbc7->latched) {
                mbc7->latched = true;
                mbc7->accel_x =
                    tilt(self->parent->joypad, BUTTON_RIGHT, BUTTON_LEFT);
                mbc7->accel_y =
                    tilt(self->parent->joypad, BUTTON_DOWN, BUTTON_UP);
            }
            break;
        }
        case 0x8: self->eeprom_write(self, value); break;
    }
}

static void update_mmm01(CartridgeClass *self)
{
    cartridge_context_t *ctx = self->context;
    mmm01_state_t *mmm01 = &ctx->mmm01;

    if (!mmm01->mapped) {
        self->map_rom(self, 0, ctx->rom_banks - 2);
        self->map_rom(self, 1, ctx->rom_banks - 1);
        self->unmap_ram(self);
        return;
    }

    uint8_t rom_low = mmm01->rom_low;
    uint8_t game_low = rom_low & (uint8_t) ~mmm01->rom_mask;
    uint8_t bank_low = game_low ? rom_low : (rom_low | 1);
    uint8_t mid = mmm01->multiplex
        ? (ctx->banking_mode ? mmm01->ram_low
                             : (mmm01->ram_low & mmm01->ram_mask))
        : mmm01->rom_mid;
    uint8_t mid_high = mmm01->multiplex ? mmm01->ram_low : mmm01->rom_mid;
    uint32_t top = mmm01->rom_high << 7;

    self->map_rom(self, 0, top | (mid << 5) | (rom_low & mmm01->rom_mask));
    self->map_rom(self, 1, top | (mid_high << 5) | bank_low);

    if (ctx->ram_enabled) {
        uint8_t ram_low = mmm01->multiplex ? mmm01->rom_mid
            : ctx->banking_mode            ? mmm01->ram_low
                                : (mmm01->ram_low & mmm01->ram_mask);
        self->map_ram(self, (mmm01->ram_high << 2) | ram_low, true, true);
    } else {
        self->unmap_ram(self);
    }
}

static void write_mmm01(CartridgeClass *self, uint16_t address, uint8_t value)
{
    cartridge_context_t *ctx = self->context;
    mmm01_state_t *mmm01 = &ctx->mmm01;

    switch (address & 0x6000) {
        case 0x0000: {
            ctx->ram_enabled = ram_enable_value(value);
            if (!mmm01->mapped) {
                mmm01->ram_mask = (value >> 4) & 0x03;
                mmm01->mapped = value & 0x40;
            }
            break;
        }
        case 0x2000: {
            mmm01->rom_low = (mmm01->rom_low & mmm01->rom_mask)
                | (value & 0x1F & ~mmm01->rom_mask);
            if (!mmm01->mapped) {
                mmm01->rom_mid = (value >> 5) & 0x03;
            }
            break;
        }
        case 0x4000: {
            mmm01->ram_low = (mmm01->ram_low & mmm01->ram_mask)
                | (value & 0x03 & ~mmm01->ram_mask);
            if (!mmm01->mapped) {
                mmm01->ram_high = (value >> 2) & 0x03;
                mmm01->rom_high = (value >> 4) & 0x03;
                mmm01->mode_locked = value & 0x40;
            }
            break;
        }
        case 0x6000: {
            if (!mmm01->mode_locked) {
                ctx->banking_mode = value & 0x01;
            }
            if (!mmm01->mapped) {
                mmm01->rom_mask = (value >> 1) & 0x1E;
                mmm01->multiplex = value & 0x40;
            }
            break;
        }
    }
    self->update_mmm01(self);
}

static void update_huc(CartridgeClass *self)
{
    cartridge_context_t *ctx = self->context;

    bool readable = ctx->mbc == MBC_HUC1
        ? !ctx->rtc_selected
        : ctx->huc3.mode == 0x0 || ctx->huc3.mode == 0xA;
    bool writable =
        ctx->mbc == MBC_HUC1 ? !ctx->rtc_selected : ctx->huc3.mode == 0xA;

    self->map_rom(self, 1, ctx->rom_bank_value);
    self->map_ram(self, ctx->ram_bank_value, readable, writable);
}

static void write_huc1(CartridgeClass *self, uint16_t address, uint8_t value)
{
    cartridge_context_t *ctx = self->context;

    switch (address & 0x6000) {
        case 0x0000: ctx->rtc_selected = (value & 0x0F) == 0x0E; break;
        case 0x2000: {
            ctx->rom_bank_value = (value & 0x3F) ? (value & 0x3F) : 1;
            break;
        }
        case 0x4000: ctx->ram_bank_value = value & 0x03; break;
        default: return;
    }
    self->update_huc(self);
}

static void huc3_update(CartridgeClass *self)
{
    huc3_state_t *huc3 = &self->context->huc3;
    uint64_t now = self->parent->context->ticks;
    uint64_t total = huc3->sub_minute + (now - huc3->last_tick);
    uint64_t minutes = total / (CLOCK_SPEED * 60ULL);

    huc3->last_tick = now;
    huc3->sub_minute = total % (CLOCK_SPEED * 60ULL);
    minutes += huc3->minutes;
    huc3->days = (huc3->days + minutes / 1440) & 0xFFF;
    huc3->minutes = minutes % 1440;
}

static void huc3_execute(CartridgeClass *self)
{
    huc3_state_t *huc3 = &self->context->huc3;
    uint8_t argument = huc3->result;

    switch (huc3->command) {
        case 0x1: {
            huc3->result = huc3->memory[huc3->address++] & 0x0F;
            return;
        }
        case 0x2: huc3->result = 0x1; return;
        case 0x3: {
            huc3->memory[huc3->address++] = argument & 0x0F;
            return;
        }
        case 0x4: huc3->address = (huc3->address & 0xF0) | argument; return;
        case 0x5: {
            huc3->address = (huc3->address & 0x0F) | (argument << 4);
            return;
        }
        case 0x6: {
            switch (argument) {
                case 0x0: {
                    self->huc3_update(self);
                    for (uint8_t i = 0; i < 3; i++) {
                        huc3->memory[i] = (huc3->minutes >> (i * 4)) & 0x0F;
                        huc3->memory[3 + i] = (huc3->days >> (i * 4)) & 0x0F;
                    }
                    huc3->memory[6] = 0;
                    break;
                }
                case 0x1: {
                    self->huc3_update(self);
                    huc3->minutes = 0;
                    huc3->days = 0;
                    for (uint8_t i = 0; i < 3; i++) {
                        huc3->minutes |= huc3->memory[i] << (i * 4);
                        huc3->days |= huc3->memory[3 + i] << (i * 4);
                    }
                    huc3->minutes %= 1440;
                    self->context->needs_save = true;
                    break;
                }
                case 0x2: huc3->result = 0x1; return;
            }
            return;
        }
    }
}

static void write_huc3(CartridgeClass *self, uint16_t address, uint8_t value)
{
    cartridge_context_t *ctx = self->context;

    switch (address & 0x6000) {
        case 0x0000: ctx->huc3.mode = value & 0x0F; break;
        case 0x2000: ctx->rom_bank_value = value & 0x7F; break;
        case 0x4000: ctx->ram_bank_value = value & 0x03; break;
        default: return;
    }
    self->update_huc(self);
}

static uint8_t read_huc_ram(CartridgeClass *self, uint16_t UNUSED address)
{
    cartridge_context_t *ctx = self->context;

    if (ctx->mbc == MBC_HUC1) {
        return 0xC0;
    }
    switch (ctx->huc3.mode) {
        case 0xC: {
            return 0x80 | ((ctx->huc3.command & 0x07) << 4)
                | (ctx->huc3.result & 0x0F);
        }
        case 0xD: return 0xFF;
        case 0xE: return 0xC0;
        default: return 0xFF;
    }
}

static void write_huc_ram(
    CartridgeClass *self, uint16_t UNUSED address, uint8_t value)
{
    huc3_state_t *huc3 = &self->context->huc3;

    if (self->context->mbc != MBC_HUC3) {
        return;
    }
    switch (huc3->mode) {
        case 0xB: {
            huc3->command = (value >> 4) & 0x07;
            huc3->result = value & 0x0F;
            break;
        }
        case 0xD: {
            if (!(value & 0x01)) {
                self->huc3_execute(self);
            }
            break;
        }
    }
}

static void update_camera(CartridgeClass *self)
{
    cartridge_context_t *ctx = self->context;

    bool ram = !ctx->camera.registers_selected;

    self->map_rom(self, 1, ctx->rom_bank_value);
    self->map_ram(self, ctx->ram_bank_value, ram, ram && ctx->ram_enabled);
}

static void write_camera(CartridgeClass *self, uint16_t address, uint8_t value)
{
    cartridge_context_t *ctx = self->context;

    switch (address & 0x6000) {
        case 0x0000: ctx->ram_enabled = ram_enable_value(value); break;
        case 0x2000: ctx->rom_bank_value = value & 0x3F; break;
        case 0x4000: {
            ctx->camera.registers_selected = value & 0x10;
            if (!ctx->camera.registers_selected) {
                ctx->ram_bank_value = value & 0x0F;
            }
            break;
        }
        default: return;
    }
    self->update_camera(self);
}

static uint8_t read_camera_ram(CartridgeClass *self, uint16_t address)
{
    if ((address & 0x7F) == 0) {
        return self->context->camera.registers[0] & 0x07;
    }
    return 0x00;
}

static void write_camera_ram(
    CartridgeClass *self, uint16_t address, uint8_t value)
{
    camera_state_t *camera = &self->context->camera;
    uint8_t reg = address & 0x7F;

    if (reg >= sizeof(camera->registers)) {
        return;
    }
    camera->registers[reg] = value;
    if (reg == 0 && (value & 0x01)) {
        memset(self->context->ram_data + 0x0100, 0x00, 0x0E00);
        camera->registers[0] &= ~0x01;
        self->context->needs_save |= self->context->has_battery;
    }
}

static void write_wisdom_tree(
    CartridgeClass *self, uint16_t address, uint8_t UNUSED value)
{
    self->context->rom_bank_value = address & 0xFF;
    self->update_mapping(self);
}

static void update_mapping(CartridgeClass *self)
{
    cartridge_context_t *ctx = self->context;

    self->map_rom(self, 0, 0);
    switch (ctx->mbc) {
        case MBC_NONE: {
            self->map_rom(self, 1, 1);
            self->map_ram(self, 0, true, true);
            break;
        }
        case MBC_1:
        case MBC_1M: self->update_mbc1(self); break;
        case MBC_3:
        case MBC_5: self->update_banks(self); break;
        case MBC_6: self->update_mbc6(self); break;
        case MBC_MMM01: self->update_mmm01(self); break;
        case MBC_HUC1:
        case MBC_HUC3: self->update_huc(self); break;
        case MBC_CAMERA: self->update_camera(self); break;
        case MBC_WISDOM_TREE: {
            self->map_rom(self, 0, ctx->rom_bank_value * 2);
            self->map_rom(self, 1, ctx->rom_bank_value * 2 + 1);
            break;
        }
        default: self->map_rom(self, 1, ctx->rom_bank_value); break;
    }
}

static bool rom_matches_logo(CartridgeClass *self, uint32_t offset)
{
    return offset + 0x134 <= self->context->rom_size
        && memcmp(self->context->rom_data + offset + 0x104,
               self->context->rom_data + 0x104, 0x30)
        == 0;
}

static void detect_mbc(CartridgeClass *self)
{
    static const uint32_t ram_sizes[] = {
        0, 0x800, 0x2000, 0x8000, 0x20000, 0x10000};
    cartridge_context_t *ctx = self->context;
    const cartridge_type_t *type = &self->types[ctx->header->type];
    const uint8_t *title = (const uint8_t *) ctx->header->title;

    ctx->mbc = type->mbc;
    ctx->has_battery = type->features & CART_BATTERY;
    ctx->has_rtc = type->features & CART_RTC;
    ctx->has_rumble = type->features & CART_RUMBLE;
    ctx->ram_size =
        ctx->header->ram_size < 6 ? ram_sizes[ctx->header->ram_size] : 0;
    if ((type->features & CART_RAM) && !ctx->ram_size) {
        ctx->ram_size = 0x2000;
    }

    if (ctx->header->type == 0xFD) {
        WARN("TAMA5 is not supported, running as ROM only");
    }
    if (ctx->mbc == MBC_1 && ctx->rom_size == 0x100000
        && rom_matches_logo(self, 0x40000)) {
        ctx->mbc = MBC_1M;
    }
    if (ctx->mbc == MBC_NONE && ctx->rom_size > 0x8000
        && (!memcmp(title, "WISDOM TREE", 11)
            || !memcmp(title, "WISDOM\0TREE", 11)
            || (ctx->header->type == 0xC0
                && ctx->header->dest_code == 0xD1))) {
        ctx->mbc = MBC_WISDOM_TREE;
    }

    switch (ctx->mbc) {
        case MBC_2: ctx->ram_size = 0x200; break;
        case MBC_6: ctx->ram_size = 0x8000; break;
        case MBC_7: ctx->ram_size = 0; break;
        case MBC_CAMERA: ctx->ram_size = 0x20000; break;
        default: break;
    }
}

static void setup_banks(CartridgeClass *self)
{
    cartridge_context_t *ctx = self->context;
    uint32_t allocation = ctx->ram_size;

    if (allocation && allocation < 0x2000 && ctx->mbc != MBC_2) {
        allocation = 0x2000;
    }
    if (allocation) {
        if (!((ctx->ram_data = calloc(allocation, 1)))) {
            HANDLE_ERROR("failed memory allocation");
        }
        ctx->ram_size = allocation;
    }
    ctx->ram_banks = allocation >= 0x2000 ? allocation / 0x2000 : 0;

    self->mbc_write = self->write_ignored;
    self->ram_read = self->read_open_bus;
    self->ram_write = self->write_ignored;
    ctx->rom_bank_value = ctx->mbc == MBC_WISDOM_TREE ? 0 : 1;
    ctx->ram_bank_value = 0;
    ctx->ram_enabled = ctx->mbc == MBC_NONE;

    switch (ctx->mbc) {
        case MBC_1:
        case MBC_1M: self->mbc_write = self->write_mbc1; break;
        case MBC_2: {
            self->mbc_write = self->write_mbc2;
            self->ram_read = self->read_mbc2_ram;
            self->ram_write = self->write_mbc2_ram;
            break;
        }
        case MBC_3: {
            self->mbc_write = self->write_mbc3;
            self->ram_read = self->read_mbc3_ram;
            self->ram_write = self->write_mbc3_ram;
            ctx->rtc.last_tick = self->parent->context->ticks;
            break;
        }
        case MBC_5: self->mbc_write = self->write_mbc5; break;
        case MBC_6: {
            if (!((ctx->mbc6.flash = malloc(MBC6_FLASH_SIZE)))) {
                HANDLE_ERROR("failed memory allocation");
            }
            memset(ctx->mbc6.flash, 0xFF, MBC6_FLASH_SIZE);
            memset(flash_status_page, 0x80, sizeof(flash_status_page));
            for (uint32_t i = 0; i < sizeof(flash_id_page); i += 0x10) {
                flash_id_page[i] = 0xC2;
                flash_id_page[i + 1] = 0x81;
            }
            ctx->mbc6.rom_bank[0] = 2;
            ctx->mbc6.rom_bank[1] = 3;
            self->mbc_write = self->write_mbc6;
            break;
        }
        case MBC_7: {
            memset(ctx->mbc7.eeprom, 0xFF, sizeof(ctx->mbc7.eeprom));
            ctx->mbc7.accel_x = 0x8000;
            ctx->mbc7.accel_y = 0x8000;
            ctx->mbc7.do_bit = true;
            self->mbc_write = self->write_mbc7;
            self->ram_read = self->read_mbc7_ram;
            self->ram_write = self->write_mbc7_ram;
            break;
        }
        case MBC_MMM01: self->mbc_write = self->write_mmm01; break;
        case MBC_HUC1:
        case MBC_HUC3: {
            self->mbc_write =
                ctx->mbc == MBC_HUC1 ? self->write_huc1 : self->write_huc3;
            self->ram_read = self->read_huc_ram;
            self->ram_write = self->write_huc_ram;
            ctx->huc3.last_tick = self->parent->context->ticks;
            break;
        }
        case MBC_CAMERA: {
            self->mbc_write = self->write_camera;
            self->ram_read = self->read_camera_ram;
            self->ram_write = self->write_camera_ram;
            break;
        }
        case MBC_WISDOM_TREE: self->mbc_write = self->write_wisdom_tree; break;
        default: break;
    }
    self->update_mapping(self);
}

static bool load(CartridgeClass *self, const char *path, const char *save)
{
    cartridge_context_t *ctx = self->context;

    snprintf(ctx->filename, sizeof(ctx->filename), "%s", path);
    snprintf(ctx->save_path, sizeof(ctx->save_path), "%s", save ? save : path);
    FILE *stream = fopen(path, "rb");
    if (!stream) {
        fprintf(stderr, "Failed to open ROM (%s)\n", path);
        return false;
    }
    char opened_msg[1100];
    snprintf(opened_msg, sizeof(opened_msg), "Opened: %s", path);
    LOG(opened_msg);
    fseek(stream, 0, SEEK_END);
    long file_size = ftell(stream);
    rewind(stream);

    if (file_size < 0x150) {
        fclose(stream);
        fprintf(stderr, "ROM file is too small (%ld bytes)\n", file_size);
        return false;
    }

    ctx->rom_banks = 2;
    while (ctx->rom_banks * 0x4000 < (uint32_t) file_size) {
        ctx->rom_banks <<= 1;
    }
    ctx->rom_size = ctx->rom_banks * 0x4000;
    if (!((ctx->rom_data = malloc(ctx->rom_size)))) {
        fclose(stream);
        HANDLE_ERROR("failed memory allocation");
    }
    memset(ctx->rom_data, 0xFF, ctx->rom_size);
    if (fread(ctx->rom_data, 1, file_size, stream) != (size_t) file_size) {
        fclose(stream);
        fprintf(stderr, "Failed to read ROM (%s)\n", path);
        return false;
    }
    fclose(stream);

    ctx->header = (rom_header_t *) (ctx->rom_data + 0x100);

    uint32_t menu = ctx->rom_size - 0x8000;
    if (ctx->rom_size > 0x8000 && ctx->rom_data[menu + 0x147] >= 0x0B
        && ctx->rom_data[menu + 0x147] <= 0x0D) {
        ctx->header = (rom_header_t *) (ctx->rom_data + menu + 0x100);
    }

    memcpy(self->title, ctx->header->title, 16);
    self->title[16] = '\0';
    if (ctx->header->cgb_flag & 0x80) {
        self->title[15] = '\0';
    }

    self->detect_mbc(self);
    ctx->needs_save = false;

    char cart_msg[512];

    LOG("Cartridge loaded");

    snprintf(cart_msg, sizeof(cart_msg), "Title: %s", self->title);
    LOG(cart_msg);

    snprintf(cart_msg, sizeof(cart_msg), "Type: %2.2X (%s)", ctx->header->type,
        self->get_rom_type(self));
    LOG(cart_msg);

    snprintf(
        cart_msg, sizeof(cart_msg), "ROM Size: %u KB", ctx->rom_size / 1024);
    LOG(cart_msg);

    snprintf(
        cart_msg, sizeof(cart_msg), "RAM Size: %u KB", ctx->ram_size / 1024);
    LOG(cart_msg);

    snprintf(cart_msg, sizeof(cart_msg), "LIC Code: %2.2X (%s)",
        ctx->header->license_code, self->get_license(self));
    LOG(cart_msg);

    snprintf(
        cart_msg, sizeof(cart_msg), "ROM Vers: %2.2X", ctx->header->version);
    LOG(cart_msg);

    self->setup_banks(self);

    uint8_t x = 0;
    uint8_t *header_bytes = (uint8_t *) ctx->header - 0x100;
    for (uint16_t i = 0x0134; i <= 0x014C; i++) {
        x = x - header_bytes[i] - 1;
    }

    snprintf(cart_msg, sizeof(cart_msg), "Checksum: %2.2X (%s)",
        ctx->header->checksum,
        x == ctx->header->checksum ? "PASSED" : "FAILED");
    LOG(cart_msg);

    if (ctx->has_battery) {
        self->load_battery(self);
    }

    return true;
}

static void serialize(CartridgeClass *self, SnapshotClass *snapshot)
{
    cartridge_context_t *ctx = self->context;
    cartridge_context_t live = *ctx;

    snapshot->field(snapshot, ctx, sizeof(*ctx));
    if (snapshot->loading) {
        memcpy(ctx->filename, live.filename, sizeof(ctx->filename));
        memcpy(ctx->save_path, live.save_path, sizeof(ctx->save_path));
        ctx->rom_data = live.rom_data;
        ctx->header = live.header;
        ctx->ram_data = live.ram_data;
        ctx->mbc6.flash = live.mbc6.flash;
        ctx->needs_save = ctx->has_battery;
    }
    if (ctx->ram_data) {
        snapshot->field(snapshot, ctx->ram_data, ctx->ram_size);
    }
    if (ctx->mbc6.flash) {
        snapshot->field(snapshot, ctx->mbc6.flash, MBC6_FLASH_SIZE);
    }
    if (snapshot->loading) {
        self->update_mapping(self);
    }
}

static void write_le(uint8_t *buffer, uint64_t value, uint8_t size)
{
    for (uint8_t i = 0; i < size; i++) {
        buffer[i] = (value >> (i * 8)) & 0xFF;
    }
}

static uint64_t read_le(const uint8_t *buffer, uint8_t size)
{
    uint64_t value = 0;
    for (uint8_t i = 0; i < size; i++) {
        value |= (uint64_t) buffer[i] << (i * 8);
    }
    return value;
}

static void restore_rtc(
    CartridgeClass *self, const uint8_t *live, uint8_t stride, uint64_t saved)
{
    mbc3_rtc_t *rtc = &self->context->rtc;
    uint64_t now = (uint64_t) time(NULL);
    uint8_t control = live[stride * 4];

    rtc->s = live[0] & 0x3F;
    rtc->m = live[stride] & 0x3F;
    rtc->h = live[stride * 2] & 0x1F;
    rtc->d = live[stride * 3] | ((control & 0x01) << 8);
    rtc->halt = control & 0x40;
    rtc->carry = control & 0x80;
    if (now > saved) {
        self->rtc_advance(self, now - saved);
    }
}

static void load_battery(CartridgeClass *self)
{
    cartridge_context_t *ctx = self->context;
    char name[1030] = {0};

    snprintf(name, sizeof(name), "%s.sav", ctx->save_path);
    FILE *stream = fopen(name, "rb");
    if (!stream) {
        LOG("No battery file found");
        return;
    }

    if (ctx->ram_data) {
        if (fread(ctx->ram_data, 1, ctx->ram_size, stream) != ctx->ram_size) {
            LOG("Battery file is shorter than the cartridge RAM");
        }
    }
    if (ctx->mbc == MBC_7) {
        fread(ctx->mbc7.eeprom, 1, MBC7_EEPROM_SIZE, stream);
    }
    if (ctx->mbc == MBC_6) {
        fread(ctx->mbc6.flash, 1, MBC6_FLASH_SIZE, stream);
    }

    uint8_t trailer[RTC_SAVE_SIZE] = {0};
    size_t size = fread(trailer, 1, sizeof(trailer), stream);
    bool rtc = ctx->mbc == MBC_3 && ctx->has_rtc;

    if (rtc && (size == RTC_SAVE_SIZE || size == RTC_SAVE_SIZE - 4)) {
        for (uint8_t i = 0; i < 5; i++) {
            ctx->rtc.latched[i] = trailer[20 + i * 4];
        }
        restore_rtc(self, trailer, 4,
            read_le(trailer + 40, size == RTC_SAVE_SIZE ? 8 : 4));
    } else if (rtc && size == LEGACY_RTC_SIZE) {
        memcpy(ctx->rtc.latched, trailer, 5);
        restore_rtc(self, trailer, 1, read_le(trailer + 5, 8));
    } else if (ctx->mbc == MBC_HUC3 && size >= HUC3_SAVE_SIZE) {
        uint64_t now = (uint64_t) time(NULL);
        uint64_t saved = read_le(trailer + 8, 8);
        uint64_t minutes = read_le(trailer, 4) % 1440
            + (now > saved ? (now - saved) / 60 : 0);
        ctx->huc3.days = (read_le(trailer + 4, 4) + minutes / 1440) & 0xFFF;
        ctx->huc3.minutes = minutes % 1440;
    }

    fclose(stream);
    LOG("Battery file loaded");
}

static size_t battery_trailer(CartridgeClass *self, uint8_t *trailer)
{
    cartridge_context_t *ctx = self->context;
    uint64_t now = (uint64_t) time(NULL);

    if (ctx->mbc == MBC_3 && ctx->has_rtc) {
        mbc3_rtc_t *rtc = &ctx->rtc;

        self->rtc_update(self);
        uint8_t live[5] = {rtc->s, rtc->m, rtc->h, rtc->d & 0xFF,
            ((rtc->d >> 8) & 0x01) | (rtc->halt ? 0x40 : 0)
                | (rtc->carry ? 0x80 : 0)};
        for (uint8_t i = 0; i < 5; i++) {
            write_le(trailer + i * 4, live[i], 4);
            write_le(trailer + 20 + i * 4, rtc->latched[i], 4);
        }
        write_le(trailer + 40, now, 8);
        return RTC_SAVE_SIZE;
    }
    if (ctx->mbc == MBC_HUC3) {
        self->huc3_update(self);
        write_le(trailer, ctx->huc3.minutes, 4);
        write_le(trailer + 4, ctx->huc3.days, 4);
        write_le(trailer + 8, now, 8);
        return HUC3_SAVE_SIZE;
    }
    return 0;
}

static void save_battery(CartridgeClass *self)
{
    cartridge_context_t *ctx = self->context;
    char name[1030] = {0};
    uint8_t trailer[RTC_SAVE_SIZE] = {0};
    size_t trailer_size = self->battery_trailer(self, trailer);
    const struct {
        const void *data;
        size_t size;
    } chunks[] = {
        {ctx->ram_data, ctx->ram_data ? ctx->ram_size : 0},
        {ctx->mbc7.eeprom, ctx->mbc == MBC_7 ? MBC7_EEPROM_SIZE : 0},
        {ctx->mbc6.flash, ctx->mbc == MBC_6 ? MBC6_FLASH_SIZE : 0},
        {trailer, trailer_size},
    };
    size_t size = 0;

    ctx->needs_save = false;
    if (!ctx->has_battery) {
        return;
    }
    for (uint8_t i = 0; i < sizeof(chunks) / sizeof(*chunks); i++) {
        size += chunks[i].size;
    }

    uint8_t *data = malloc(size);
    uint8_t *cursor = data;
    if (!data) {
        HANDLE_ERROR("failed memory allocation");
    }
    for (uint8_t i = 0; i < sizeof(chunks) / sizeof(*chunks); i++) {
        memcpy(cursor, chunks[i].data, chunks[i].size);
        cursor += chunks[i].size;
    }

    snprintf(name, sizeof(name), "%s.sav", ctx->save_path);
    FILE *stream = fopen(name, "wb");
    if (stream) {
        fwrite(data, 1, size, stream);
        fclose(stream);
    } else {
        LOG("Failed to open battery file");
    }
    free(data);
}

const CartridgeClass init_cartridge = {
    {
        ._size = sizeof(CartridgeClass),
        ._name = "Cartridge",
        ._constructor = constructor,
        ._destructor = destructor,
    },
    .types =
        {
            [0x00] = {"ROM ONLY", MBC_NONE, 0},
            [0x01] = {"MBC1", MBC_1, 0},
            [0x02] = {"MBC1+RAM", MBC_1, CART_RAM},
            [0x03] = {"MBC1+RAM+BATTERY", MBC_1, CART_RAM | CART_BATTERY},
            [0x05] = {"MBC2", MBC_2, 0},
            [0x06] = {"MBC2+BATTERY", MBC_2, CART_BATTERY},
            [0x08] = {"ROM+RAM", MBC_NONE, CART_RAM},
            [0x09] = {"ROM+RAM+BATTERY", MBC_NONE, CART_RAM | CART_BATTERY},
            [0x0B] = {"MMM01", MBC_MMM01, 0},
            [0x0C] = {"MMM01+RAM", MBC_MMM01, CART_RAM},
            [0x0D] = {"MMM01+RAM+BATTERY", MBC_MMM01, CART_RAM | CART_BATTERY},
            [0x0F] = {"MBC3+TIMER+BATTERY", MBC_3, CART_BATTERY | CART_RTC},
            [0x10] = {"MBC3+TIMER+RAM+BATTERY", MBC_3,
                CART_RAM | CART_BATTERY | CART_RTC},
            [0x11] = {"MBC3", MBC_3, 0},
            [0x12] = {"MBC3+RAM", MBC_3, CART_RAM},
            [0x13] = {"MBC3+RAM+BATTERY", MBC_3, CART_RAM | CART_BATTERY},
            [0x19] = {"MBC5", MBC_5, 0},
            [0x1A] = {"MBC5+RAM", MBC_5, CART_RAM},
            [0x1B] = {"MBC5+RAM+BATTERY", MBC_5, CART_RAM | CART_BATTERY},
            [0x1C] = {"MBC5+RUMBLE", MBC_5, CART_RUMBLE},
            [0x1D] = {"MBC5+RUMBLE+RAM", MBC_5, CART_RAM | CART_RUMBLE},
            [0x1E] = {"MBC5+RUMBLE+RAM+BATTERY", MBC_5,
                CART_RAM | CART_BATTERY | CART_RUMBLE},
            [0x20] = {"MBC6", MBC_6, CART_RAM | CART_BATTERY},
            [0x22] = {"MBC7+SENSOR+RUMBLE+RAM+BATTERY", MBC_7,
                CART_BATTERY | CART_RUMBLE},
            [0xFC] = {"POCKET CAMERA", MBC_CAMERA, CART_RAM | CART_BATTERY},
            [0xFD] = {"BANDAI TAMA5", MBC_NONE, 0},
            [0xFE] = {"HuC3", MBC_HUC3, CART_RAM | CART_BATTERY | CART_RTC},
            [0xFF] = {"HuC1+RAM+BATTERY", MBC_HUC1, CART_RAM | CART_BATTERY},
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
    .detect_mbc = detect_mbc,
    .setup_banks = setup_banks,
    .update_mapping = update_mapping,
    .serialize = serialize,
    .map_rom = map_rom,
    .map_ram = map_ram,
    .unmap_ram = unmap_ram,
    .mbc_write = write_ignored,
    .ram_read = read_open_bus,
    .ram_write = write_ignored,
    .write_mbc1 = write_mbc1,
    .write_mbc2 = write_mbc2,
    .write_mbc3 = write_mbc3,
    .write_mbc5 = write_mbc5,
    .write_mbc6 = write_mbc6,
    .write_mbc7 = write_mbc7,
    .write_mmm01 = write_mmm01,
    .write_huc1 = write_huc1,
    .write_huc3 = write_huc3,
    .write_camera = write_camera,
    .write_wisdom_tree = write_wisdom_tree,
    .update_mbc1 = update_mbc1,
    .update_banks = update_banks,
    .update_mbc6 = update_mbc6,
    .update_mmm01 = update_mmm01,
    .update_huc = update_huc,
    .update_camera = update_camera,
    .read_mbc2_ram = read_mbc2_ram,
    .write_mbc2_ram = write_mbc2_ram,
    .read_mbc3_ram = read_mbc3_ram,
    .write_mbc3_ram = write_mbc3_ram,
    .read_mbc7_ram = read_mbc7_ram,
    .write_mbc7_ram = write_mbc7_ram,
    .read_huc_ram = read_huc_ram,
    .write_huc_ram = write_huc_ram,
    .read_camera_ram = read_camera_ram,
    .write_camera_ram = write_camera_ram,
    .read_open_bus = read_open_bus,
    .write_ignored = write_ignored,
    .flash_command = flash_command,
    .eeprom_write = eeprom_write,
    .eeprom_command = eeprom_command,
    .rtc_update = rtc_update,
    .rtc_advance = rtc_advance,
    .latch_rtc = latch_rtc,
    .read_rtc = read_rtc,
    .write_rtc = write_rtc,
    .huc3_update = huc3_update,
    .huc3_execute = huc3_execute,
    .load_battery = load_battery,
    .battery_trailer = battery_trailer,
    .save_battery = save_battery,
};

const class_t *Cartridge = (const class_t *) &init_cartridge;
