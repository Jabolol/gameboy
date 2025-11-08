#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    IOClass *self = (IOClass *) ptr;
    self->parent = va_arg(*args, GameboyClass *);
}

static uint8_t read(IOClass *self, uint16_t address)
{
    switch (address) {
        case JOYPAD: {
            return self->parent->joypad->output(self->parent->joypad);
        }
        case SERIAL_DATA: {
            return self->serial_data[0];
        }
        case SERIAL_CONTROL: {
            return self->serial_data[1];
        }
        case TIMER_RANGE: {
            return self->parent->timer->read(self->parent->timer, address);
        }
        case INTERRUPT_FLAG: {
            return self->parent->cpu->get_int_flags(self->parent->cpu);
        }
        case SOUND_RANGE: {
            return self->parent->sound->read(self->parent->sound, address);
        }
        case LCD_RANGE: {
            return self->parent->lcd->read(self->parent->lcd, address);
        }
        case LCD_OPRI: {
            if (self->parent->context->hw_mode == HW_CGB) {
                return self->parent->lcd->context->opri;
            }
            return 0xFF;
        }
        case LCD_SVBK: {
            return self->parent->ram->context->wram_bank | 0xF8;
        }
        case LCD_UNDOC_FF72: {
            if (self->parent->context->hw_mode == HW_CGB) {
                return self->parent->lcd->context->undoc_ff72;
            }
            return 0xFF;
        }
        case LCD_UNDOC_FF73: {
            if (self->parent->context->hw_mode == HW_CGB) {
                return self->parent->lcd->context->undoc_ff73;
            }
            return 0xFF;
        }
        case LCD_UNDOC_FF74: {
            if (self->parent->context->hw_mode == HW_CGB) {
                return self->parent->lcd->context->undoc_ff74;
            }
            return 0xFF;
        }
        case LCD_UNDOC_FF75: {
            if (self->parent->context->hw_mode == HW_CGB) {
                return self->parent->lcd->context->undoc_ff75 | 0x8F;
            }
            return 0xFF;
        }
        case LCD_PCM12:
        case LCD_PCM34: {
            return 0x00;
        }
        default: {
            NOT_IMPLEMENTED();
            return 0;
        }
    }
}

static void write(IOClass *self, uint16_t address, uint8_t value)
{
    switch (address) {
        case JOYPAD: {
            self->parent->joypad->choose(self->parent->joypad, value);
            break;
        }
        case SERIAL_DATA: {
            self->serial_data[0] = value;
            break;
        }
        case SERIAL_CONTROL: {
            self->serial_data[1] = value;
            break;
        }
        case TIMER_RANGE: {
            self->parent->timer->write(self->parent->timer, address, value);
            break;
        }
        case INTERRUPT_FLAG: {
            self->parent->cpu->set_int_flags(self->parent->cpu, value);
            break;
        }
        case SOUND_RANGE: {
            self->parent->sound->write(self->parent->sound, address, value);
            break;
        }
        case LCD_RANGE: {
            self->parent->lcd->write(self->parent->lcd, address, value);
            break;
        }
        case LCD_OPRI: {
            if (self->parent->context->hw_mode == HW_CGB) {
                self->parent->lcd->context->opri =
                    (self->parent->lcd->context->opri & 0xFE) | (value & 0x01);
            }
            break;
        }
        case LCD_SVBK: {
            if (self->parent->context->hw_mode == HW_CGB) {
                self->parent->ram->context->wram_bank = value & 0x07;
                if (self->parent->ram->context->wram_bank == 0) {
                    self->parent->ram->context->wram_bank = 1;
                }
            }
            break;
        }
        case LCD_UNDOC_FF72: {
            if (self->parent->context->hw_mode == HW_CGB) {
                self->parent->lcd->context->undoc_ff72 = value;
            }
            break;
        }
        case LCD_UNDOC_FF73: {
            if (self->parent->context->hw_mode == HW_CGB) {
                self->parent->lcd->context->undoc_ff73 = value;
            }
            break;
        }
        case LCD_UNDOC_FF74: {
            if (self->parent->context->hw_mode == HW_CGB) {
                self->parent->lcd->context->undoc_ff74 = value;
            }
            break;
        }
        case LCD_UNDOC_FF75: {
            if (self->parent->context->hw_mode == HW_CGB) {
                self->parent->lcd->context->undoc_ff75 = value & 0x70;
            }
            break;
        }
        case LCD_PCM12:
        case LCD_PCM34: {
            break;
        }
        default: {
            NOT_IMPLEMENTED();
        }
    }
}

const IOClass init_io = {
    {
        ._size = sizeof(IOClass),
        ._name = "IO",
        ._constructor = constructor,
        ._destructor = NULL,
    },
    .read = read,
    .write = write,
};

const class_t *IO = (const class_t *) &init_io;
