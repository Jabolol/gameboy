#include "bus.h"
#include "common.h"
#include "instructions.h"
#include "oop.h"

#ifndef __CPU
    #define __CPU

typedef struct gameboy_aux GameboyClass;
typedef struct cpu_aux CPUClass;

typedef struct cpu_aux {
    /* Properties */
    class_t metadata;
    cpu_context_t *context;
    GameboyClass *parent;
    BusClass *bus;
    InstructionsClass *instructions;
    /* Methods */
    void (*init)(CPUClass *);
    bool (*step)(CPUClass *);
    void (*fetch_instructions)(CPUClass *);
    void (*fetch_data)(CPUClass *);
    void (*execute)(CPUClass *);
    void (*set_flags)(CPUClass *, char, char, char, char);
    bool (*check_condition)(CPUClass *);
    uint16_t (*reverse)(uint16_t);
    uint16_t (*read_register)(CPUClass *, register_type_t);
    void (*set_register)(CPUClass *, register_type_t, uint16_t);
    void (*set_ie_register)(CPUClass *, uint8_t);
    uint8_t (*get_ie_register)(CPUClass *);
    registers_t *(*get_registers)(CPUClass *);
    bool (*is_16bit)(register_type_t);
} CPUClass;

extern const class_t *CPU;
#endif
