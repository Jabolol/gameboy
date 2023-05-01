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
    register_type_t register_lookup[8];
    /* Methods */
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
    register_type_t (*decode_register)(CPUClass *, uint8_t);
    uint8_t (*read_register8)(CPUClass *, register_type_t);
    void (*set_register8)(CPUClass *, register_type_t, uint8_t);
    uint8_t (*get_int_flags)(CPUClass *);
    void (*set_int_flags)(CPUClass *, uint8_t);
    void (*int_handle)(CPUClass *, uint16_t);
    bool (*int_check)(CPUClass *, uint16_t, interrupt_t);
    void (*request_interrupt)(CPUClass *, interrupt_t);
    void (*handle_interrupts)(CPUClass *);
} CPUClass;

extern const class_t *CPU;
#endif
