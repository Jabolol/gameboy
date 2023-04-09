#include <stdint.h>

#ifndef __COMMON
    #define __COMMON
    #define ANSI_COLOR_RED   "\x1b[31m"
    #define ANSI_COLOR_RESET "\x1b[0m"
    #define HANDLE_ERROR(e)                                                   \
        do {                                                                  \
            fprintf(stderr, "Error: %s: %s:%d %s()\n", e, __FILE__, __LINE__, \
                __func__);                                                    \
            exit(EXIT_FAILURE);                                               \
        } while (0)
    #define NOT_IMPLEMENTED()                                                \
        do {                                                                 \
            fprintf(stderr,                                                  \
                ANSI_COLOR_RED                                               \
                "Error: %s:%d %s() - not implemented" ANSI_COLOR_RESET "\n", \
                __FILE__, __LINE__, __func__);                               \
        } while (0)

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
#endif
