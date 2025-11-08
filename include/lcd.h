#include "common.h"
#include "oop.h"

#ifndef __LCD
    #define __LCD

typedef struct gameboy_aux GameboyClass;
typedef struct lcd_aux LCDClass;

typedef struct lcd_aux {
    /* Properties */
    class_t metadata;
    GameboyClass *parent;
    uint64_t default_colors[4];
    lcd_context_t *context;
    /* Methods */
    uint8_t (*read)(LCDClass *, uint16_t);
    void (*write)(LCDClass *, uint16_t, uint8_t);
    void (*update)(LCDClass *, uint8_t, uint8_t);
    void (*hdma_start)(LCDClass *, uint8_t);
    void (*hdma_tick)(LCDClass *);
} LCDClass;

extern const class_t *LCD;
#endif
