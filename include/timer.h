#include "common.h"
#include "oop.h"

#ifndef __TIMER
    #define __TIMER

typedef struct gameboy_aux GameboyClass;
typedef struct timer_aux TimerClass;
typedef struct snapshot_aux SnapshotClass;

typedef struct timer_aux {
    /* Properties */
    class_t metadata;
    timer_context_t *context;
    GameboyClass *parent;
    const uint16_t tac_bits[4];
    /* Methods */
    void (*tick)(TimerClass *);
    uint8_t (*read)(TimerClass *, uint16_t);
    void (*write)(TimerClass *, uint16_t, uint8_t);
    void (*set_div)(TimerClass *, uint16_t);
    bool (*timer_input)(TimerClass *, uint16_t, uint8_t);
    void (*increment_tima)(TimerClass *);
    void (*serialize)(TimerClass *, SnapshotClass *);
} TimerClass;

extern const class_t *Timer;
#endif
