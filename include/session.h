#include "gameboy.h"

#ifndef __SESSION
    #define __SESSION

typedef struct session_aux SessionClass;

typedef struct session_aux {
    /* Properties */
    class_t metadata;
    GameboyClass *gameboy;
    pthread_t thread;
    /* Methods */
    GameboyClass *(*get)(SessionClass *);
    void (*set)(SessionClass *, GameboyClass *);
} SessionClass;

extern const class_t *Session;
#endif
