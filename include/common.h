#include <stdint.h>

#ifndef __COMMON
    #define __COMMON
    #define ANSI_COLOR_RED    "\x1b[31m"
    #define ANSI_COLOR_YELLOW "\x1b[33m"
    #define ANSI_COLOR_RESET  "\x1b[0m"
    #define HANDLE_ERROR(e)                                                 \
        do {                                                                \
            fprintf(stderr,                                                 \
                ANSI_COLOR_RED "Error: %s:%d %s() - %s\n" ANSI_COLOR_RESET, \
                __FILE__, __LINE__, __func__, e);                           \
            exit(EXIT_FAILURE);                                             \
        } while (0)
    #define NOT_IMPLEMENTED()                                               \
        do {                                                                \
            fprintf(stderr,                                                 \
                ANSI_COLOR_YELLOW                                           \
                "Warn: %s:%d %s() - not implemented" ANSI_COLOR_RESET "\n", \
                __FILE__, __LINE__, __func__);                              \
        } while (0)
    #define BIT(a, n) ((a & (1 << n)) ? 1 : 0)
    #define BIT_SET(a, n, on)   \
        {                       \
            if (on)             \
                a |= (1 << n);  \
            else                \
                a &= ~(1 << n); \
        }
    #define BETWEEN(a, b, c) ((a >= b) && (a <= c))
    #define UNUSED           __attribute__((unused))

typedef struct {
    bool paused;
    bool running;
    uint64_t ticks;
} emulator_context_t;

typedef struct {
    uint8_t entry[4];
    uint8_t logo[0x30];
    char title[16];
    uint16_t new_license_code;
    uint8_t sgb_flag;
    uint8_t type;
    uint8_t rom_size;
    uint8_t ram_size;
    uint8_t dest_code;
    uint8_t license_code;
    uint8_t version;
    uint8_t checksum;
    uint16_t global_checksum;
} rom_header_t;

typedef struct {
    char filename[1024];
    uint32_t rom_size;
    uint8_t *rom_data;
    rom_header_t *header;
} cartridge_context_t;

typedef struct {
    uint8_t a;
    uint8_t f;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t h;
    uint8_t l;
    uint16_t pc;
    uint16_t sp;
} registers_t;

typedef enum {
    AM_IMP,
    AM_R_D16,
    AM_R_R,
    AM_MR_R,
    AM_R,
    AM_R_D8,
    AM_R_MR,
    AM_R_HLI,
    AM_R_HLD,
    AM_HLI_R,
    AM_HLD_R,
    AM_R_A8,
    AM_A8_R,
    AM_HL_SPR,
    AM_D16,
    AM_D8,
    AM_D16_R,
    AM_MR_D8,
    AM_MR,
    AM_A16_R,
    AM_R_A16
} address_mode_t;

typedef enum {
    RT_NONE,
    RT_A,
    RT_F,
    RT_B,
    RT_C,
    RT_D,
    RT_E,
    RT_H,
    RT_L,
    RT_AF,
    RT_BC,
    RT_DE,
    RT_HL,
    RT_SP,
    RT_PC,
} register_type_t;

typedef enum {
    IN_NONE,
    IN_NOP,
    IN_LD,
    IN_INC,
    IN_DEC,
    IN_RLCA,
    IN_ADD,
    IN_RRCA,
    IN_STOP,
    IN_RLA,
    IN_JR,
    IN_RRA,
    IN_DAA,
    IN_CPL,
    IN_SCF,
    IN_CCF,
    IN_HALT,
    IN_ADC,
    IN_SUB,
    IN_SBC,
    IN_AND,
    IN_XOR,
    IN_OR,
    IN_CP,
    IN_POP,
    IN_JP,
    IN_PUSH,
    IN_RET,
    IN_CB,
    IN_CALL,
    IN_RETI,
    IN_LDH,
    IN_JPHL,
    IN_DI,
    IN_EI,
    IN_RST,
    IN_ERR,
    IN_RLC,
    IN_RRC,
    IN_RL,
    IN_RR,
    IN_SLA,
    IN_SRA,
    IN_SWAP,
    IN_SRL,
    IN_BIT,
    IN_RES,
    IN_SET,
} instruction_type_t;

typedef enum {
    CT_NONE,
    CT_NZ,
    CT_Z,
    CT_NC,
    CT_C,
} condition_type_t;

typedef struct {
    instruction_type_t type;
    address_mode_t mode;
    register_type_t register_1;
    register_type_t register_2;
    condition_type_t condition;
    uint8_t parameter;
} instruction_t;

typedef struct {
    registers_t registers;
    uint16_t fetched_data;
    uint16_t mem_dest;
    bool dest_is_mem;
    uint8_t opcode;
    instruction_t *inst;
    bool halted;
    bool stepping;
    bool int_master_enabled;
} cpu_context_t;

typedef struct cpu_aux CPUClass;

typedef void (*proc_fn)(CPUClass *);

#endif
