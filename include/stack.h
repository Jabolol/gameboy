#include "common.h"
#include "cpu.h"
#include "oop.h"

#ifndef __STACK
    #define __STACK

typedef struct stack_aux StackClass;

typedef struct stack_aux {
    /* Properties */
    class_t metadata;
    CPUClass *cpu;
    /* Methods */
    void (*push)(StackClass *, uint8_t);
    void (*push16)(StackClass *, uint16_t);
    uint8_t (*pop)(StackClass *);
    uint16_t (*pop16)(StackClass *);
} StackClass;

extern const class_t *Stack;
#endif
