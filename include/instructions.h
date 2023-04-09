#include "common.h"
#include "oop.h"

#ifndef __INSTRUCTIONS
    #define __INSTRUCTIONS

typedef struct instructions_aux InstructionsClass;

typedef struct instructions_aux {
    /* Properties */
    class_t metadata;
    instruction_t instructions[0x100];
    char *lookup_table[48];
    proc_fn processors[34];
    /* Methods */
    proc_fn (*get_proc)(InstructionsClass *, instruction_type_t);
    instruction_t *(*by_opcode)(InstructionsClass *, uint8_t);
    char *(*lookup)(InstructionsClass *, instruction_type_t);
} InstructionsClass;

extern const class_t *Instructions;
#endif
