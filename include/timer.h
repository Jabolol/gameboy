#include "common.h"
#include "oop.h"

#ifndef __TIMER
    #define __TIMER

typedef struct gameboy_aux GameboyClass;
typedef struct timer_aux TimerClass;

typedef struct timer_aux {
    /* Properties */
    class_t metadata;
    timer_context_t *context;
    GameboyClass *parent;
    /* Methods */
    void (*tick)(TimerClass *);
    uint8_t (*read)(TimerClass *, uint16_t);
    void (*write)(TimerClass *, uint16_t, uint8_t);
} TimerClass;

extern const class_t *Timer;
#endif
