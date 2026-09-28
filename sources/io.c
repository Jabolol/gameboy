#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    IOClass *self = (IOClass *) ptr;
    if (!((self->serial = calloc(1, sizeof(*self->serial))))) {
        HANDLE_ERROR("failed memory allocation");
    }
    self->parent = va_arg(*args, GameboyClass *);
}

static void destructor(void *ptr)
{
    IOClass *self = (IOClass *) ptr;
    free(self->serial);
}

static void serial_start(IOClass *self, uint8_t value)
{
    bool cgb = self->parent->context->hw_mode == HW_CGB;

    self->serial->control = value | (cgb ? 0x7C : 0x7E);
    if ((value & 0x81) != 0x81) {
        self->serial->bits_remaining = 0;
        return;
    }
    if (self->serial->log_size < SERIAL_LOG_SIZE - 1) {
        self->serial->log[self->serial->log_size++] = self->serial->data;
        self->serial->log[self->serial->log_size] = '\0';
    }
    self->serial->bits_remaining = 8;
    self->serial->previous_div = self->parent->timer->context->div;
}

static void tick(IOClass *self)
{
    serial_context_t *serial = self->serial;
    uint16_t div = self->parent->timer->context->div;
    uint16_t previous = serial->previous_div;

    serial->previous_div = div;
    if (!serial->bits_remaining) {
        return;
    }

    bool fast =
        self->parent->context->hw_mode == HW_CGB && (serial->control & 0x02);
    uint16_t clock_bit = fast ? 0x0008 : 0x0100;
    if (!(previous & clock_bit) || (div & clock_bit)) {
        return;
    }

    serial->data = (serial->data << 1) | 0x01;
    if (--serial->bits_remaining == 0) {
        serial->control &= 0x7F;
        self->parent->cpu->request_interrupt(self->parent->cpu, IT_SERIAL);
    }
}

static uint8_t read(IOClass *self, uint16_t address)
{
    bool cgb = self->parent->context->hw_mode == HW_CGB;

    switch (address) {
        case JOYPAD: return self->parent->joypad->output(self->parent->joypad);
        case SERIAL_DATA: return self->serial->data;
        case SERIAL_CONTROL:
            return self->serial->control | (cgb ? 0x7C : 0x7E);
        case TIMER_RANGE:
            return self->parent->timer->read(self->parent->timer, address);
        case INTERRUPT_FLAG:
            return self->parent->cpu->get_int_flags(self->parent->cpu) | 0xE0;
        case SOUND_RANGE:
            return self->parent->sound->read(self->parent->sound, address);
        case LCD_RANGE:
        case LCD_OPRI: {
            if (address != KEY1) {
                return self->parent->lcd->read(self->parent->lcd, address);
            }
            if (!cgb) {
                return 0xFF;
            }
            return (self->parent->context->double_speed ? 0x80 : 0x00)
                | (self->parent->context->speed_switch_armed ? 0x01 : 0x00)
                | 0x7E;
        }
        case LCD_SVBK: {
            if (!cgb) {
                return 0xFF;
            }
            return self->parent->ram->context->wram_bank | 0xF8;
        }
        case LCD_UNDOC_FF72: {
            return cgb ? self->parent->lcd->context->undoc_ff72 : 0xFF;
        }
        case LCD_UNDOC_FF73: {
            return cgb ? self->parent->lcd->context->undoc_ff73 : 0xFF;
        }
        case LCD_UNDOC_FF74: {
            return cgb ? self->parent->lcd->context->undoc_ff74 : 0xFF;
        }
        case LCD_UNDOC_FF75: {
            return cgb ? (self->parent->lcd->context->undoc_ff75 | 0x8F)
                       : 0xFF;
        }
        case LCD_PCM12:
        case LCD_PCM34: {
            if (!cgb) {
                return 0xFF;
            }
            return self->parent->sound->pcm(
                self->parent->sound, address == LCD_PCM34);
        }
        default: return 0xFF;
    }
}

static void write(IOClass *self, uint16_t address, uint8_t value)
{
    bool cgb = self->parent->context->hw_mode == HW_CGB;

    switch (address) {
        case JOYPAD: {
            self->parent->joypad->choose(self->parent->joypad, value);
            break;
        }
        case SERIAL_DATA: {
            self->serial->data = value;
            break;
        }
        case SERIAL_CONTROL: {
            self->serial_start(self, value);
            break;
        }
        case TIMER_RANGE: {
            self->parent->timer->write(self->parent->timer, address, value);
            break;
        }
        case INTERRUPT_FLAG: {
            self->parent->cpu->set_int_flags(self->parent->cpu, value & 0x1F);
            break;
        }
        case SOUND_RANGE: {
            self->parent->sound->write(self->parent->sound, address, value);
            break;
        }
        case LCD_RANGE:
        case LCD_OPRI: {
            if (address != KEY1) {
                self->parent->lcd->write(self->parent->lcd, address, value);
            } else if (cgb) {
                self->parent->context->speed_switch_armed = value & 0x01;
            }
            break;
        }
        case LCD_SVBK: {
            if (cgb) {
                self->parent->ram->context->wram_bank = value & 0x07;
                if (self->parent->ram->context->wram_bank == 0) {
                    self->parent->ram->context->wram_bank = 1;
                }
            }
            break;
        }
        case LCD_UNDOC_FF72: {
            if (cgb) {
                self->parent->lcd->context->undoc_ff72 = value;
            }
            break;
        }
        case LCD_UNDOC_FF73: {
            if (cgb) {
                self->parent->lcd->context->undoc_ff73 = value;
            }
            break;
        }
        case LCD_UNDOC_FF74: {
            if (cgb) {
                self->parent->lcd->context->undoc_ff74 = value;
            }
            break;
        }
        case LCD_UNDOC_FF75: {
            if (cgb) {
                self->parent->lcd->context->undoc_ff75 = value & 0x70;
            }
            break;
        }
        default: break;
    }
}

static void serialize(IOClass *self, SnapshotClass *snapshot)
{
    snapshot->field(snapshot, self->serial, sizeof(*self->serial));
}

const IOClass init_io = {
    {
        ._size = sizeof(IOClass),
        ._name = "IO",
        ._constructor = constructor,
        ._destructor = destructor,
    },
    .read = read,
    .write = write,
    .tick = tick,
    .serial_start = serial_start,
    .serialize = serialize,
};

const class_t *IO = (const class_t *) &init_io;
