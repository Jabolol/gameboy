#include "oop.h"

#ifndef __CPU
    #define __CPU

typedef struct cpu_aux CPUClass;

typedef struct cpu_aux {
    /* Properties */
    class_t metadata;
    /* Methods */
    void (*init)(CPUClass *);
    bool (*step)(CPUClass *);
} CPUClass;

extern const class_t *CPU;
#endif
