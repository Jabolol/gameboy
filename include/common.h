#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef __COMMON
    #define __COMMON
    #define ANSI_COLOR_BLACK          "\x1b[30m"
    #define ANSI_COLOR_RED            "\x1b[31m"
    #define ANSI_COLOR_GREEN          "\x1b[32m"
    #define ANSI_COLOR_YELLOW         "\x1b[33m"
    #define ANSI_COLOR_BLUE           "\x1b[34m"
    #define ANSI_COLOR_MAGENTA        "\x1b[35m"
    #define ANSI_COLOR_CYAN           "\x1b[36m"
    #define ANSI_COLOR_WHITE          "\x1b[37m"
    #define ANSI_COLOR_BRIGHT_BLACK   "\x1b[90m"
    #define ANSI_COLOR_BRIGHT_RED     "\x1b[91m"
    #define ANSI_COLOR_BRIGHT_GREEN   "\x1b[92m"
    #define ANSI_COLOR_BRIGHT_YELLOW  "\x1b[93m"
    #define ANSI_COLOR_BRIGHT_BLUE    "\x1b[94m"
    #define ANSI_COLOR_BRIGHT_MAGENTA "\x1b[95m"
    #define ANSI_COLOR_BRIGHT_CYAN    "\x1b[96m"
    #define ANSI_COLOR_BRIGHT_WHITE   "\x1b[97m"
    #define ANSI_COLOR_RESET          "\x1b[0m"
    #define ANSI_COLOR_BOLD           "\x1b[1m"
    #define ANSI_COLOR_UNDERLINE      "\x1b[4m"
    #define ANSI_COLOR_BLINK          "\x1b[5m"
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
    #define LOG(m)                                                        \
        do {                                                              \
            fprintf(stdout,                                               \
                ANSI_COLOR_MAGENTA "Info: %s:%d %s() - " ANSI_COLOR_GREEN \
                                   "%s" ANSI_COLOR_RESET "\n",            \
                __FILE__, __LINE__, __func__, m);                         \
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
    #define ROM_RANGE        0 ... 0x7FFF
    #define CHAR_RANGE       0x8000 ... 0x9FFF
    #define CART_RAM_RANGE   0xA000 ... 0xBFFF
    #define WRAM_RANGE       0xC000 ... 0xDFFF
    #define ECHO_RANGE       0xE000 ... 0xFDFF
    #define OAM_RANGE        0xFE00 ... 0xFE9F
    #define RESERVED_RANGE   0xFEA0 ... 0xFEFF
    #define IO_REGS_RANGE    0xFF00 ... 0xFF7F
    #define HRAM_RANGE       0xFF80 ... 0xFFFE
    #define CPU_ENABLE_REG   0xFFFF
    #define CB_BIT           1
    #define CB_RST           2
    #define CB_SET           3
    #define CB_RLC           0
    #define CB_RRC           1
    #define CB_RL            2
    #define CB_RR            3
    #define CB_SLA           4
    #define CB_SRA           5
    #define CB_SWP           6
    #define CB_SRL           7
    #define LOOKUP_REG1      self->str_register_lookup[instruction->register_1]
    #define LOOKUP_REG2      self->str_register_lookup[instruction->register_2]
    #define WIDTH            768
    #define HEIGHT           576
    #define SCALE            3
    #define JOYPAD           0xFF00
    #define SERIAL_DATA      0xFF01
    #define SERIAL_CONTROL   0xFF02
    #define FIRST_LAST_SET   0b10000001
    #define DIV              0xFF04
    #define TIMA             0xFF05
    #define TMA              0xFF06
    #define TAC              0xFF07
    #define TIMER_RANGE      0xFF04 ... 0xFF07
    #define LCD_RANGE        0xFF40 ... 0xFF6B
    #define SOUND_RANGE      0xFF10 ... 0xFF3F
    #define INTERRUPT_FLAG   0xFF0F
    #define TRANSFER_REG     0xFF46
    #define LCD_OPRI         0xFF6C
    #define LCD_UNDOC_FF72   0xFF72
    #define LCD_UNDOC_FF73   0xFF73
    #define LCD_UNDOC_FF74   0xFF74
    #define LCD_UNDOC_FF75   0xFF75
    #define LCD_PCM12        0xFF76
    #define LCD_PCM34        0xFF77
    #define INST_BUFF_LEN    16
    #define START_LOCATION   0x8000
    #define LCD_Y_COORD      0xFF44
    #define LCD_CONTROL      0xFF40
    #define LCD_BG_PAL       0xFF47
    #define LCD_S1_PAL       0xFF48
    #define LCD_S2_PAL       0xFF49
    #define KEY1             0xFF4D
    #define LCD_VBK          0xFF4F
    #define LCD_HDMA1        0xFF51
    #define LCD_HDMA2        0xFF52
    #define LCD_HDMA3        0xFF53
    #define LCD_HDMA4        0xFF54
    #define LCD_HDMA5        0xFF55
    #define LCD_BCPS         0xFF68
    #define LCD_BCPD         0xFF69
    #define LCD_OCPS         0xFF6A
    #define LCD_OCPD         0xFF6B
    #define LCD_SVBK         0xFF70
    #define HBLANK_OFF       3
    #define VBLANK_OFF       4
    #define OAM_OFF          5
    #define LYC_OFF          6
    #define BUFFER_SIZE      256
    #define LINES_PER_FRAME  154
    #define TICKS_PER_LINE   456
    #define Y_RES            144
    #define X_RES            160
    #define FPS              60
    #define MAX_FIFO_ITEMS   8
    #define OAM_ENTRIES      40
    #define MAX_SPRITES      10
    #define LCDC_BGW_ENABLE  (BIT(self->parent->lcd->context->control, 0))
    #define LCDC_OBJ_ENABLE  (BIT(self->parent->lcd->context->control, 1))
    #define LCDC_OBJ_HEIGHT \
        (BIT(self->parent->lcd->context->control, 2) ? 16 : 8)
    #define LCDC_BG_MAP_AREA \
        (BIT(self->parent->lcd->context->control, 3) ? 0x9C00 : 0x9800)
    #define LCDC_BGW_DATA_AREA \
        (BIT(self->parent->lcd->context->control, 4) ? 0x8000 : 0x8800)
    #define LCDC_WIN_ENABLE (BIT(self->parent->lcd->context->control, 5))
    #define LCDC_WIN_MAP_AREA \
        (BIT(self->parent->lcd->context->control, 6) ? 0x9C00 : 0x9800)
    #define LCDC_LCD_ENABLE   (BIT(self->parent->lcd->context->control, 7))
    #define CPU_FLAG_Z        BIT(cpu->context->registers.f, 7)
    #define CPU_FLAG_N        BIT(cpu->context->registers.f, 6)
    #define CPU_FLAG_H        BIT(cpu->context->registers.f, 5)
    #define CPU_FLAG_C        BIT(cpu->context->registers.f, 4)
    #define AUDIO_FREQUENCY   44100
    #define AUDIO_FORMAT      AUDIO_S16SYS
    #define AUDIO_CHANNELS    2
    #define AUDIO_SAMPLES     1024
    #define AUDIO_MAX_SAMPLES 4096

typedef enum { HW_DMG, HW_CGB } hardware_mode_t;

typedef struct {
    bool paused;
    bool running;
    bool die;
    uint64_t ticks;
    uint32_t prev_frame;
    hardware_mode_t hw_mode;
    bool double_speed;
    bool speed_switch_armed;
    uint16_t stop_cycles_remaining;
} emulator_context_t;

typedef struct {
    uint8_t entry[4];
    uint8_t logo[0x30];
    char title[15];
    uint8_t cgb_flag;
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
    bool ram_enabled;
    bool ram_banking;
    uint8_t *rom_bank_x;
    uint8_t banking_mode;
    uint16_t rom_bank_value;
    uint8_t ram_bank_value;
    uint8_t *ram_bank;
    uint8_t *ram_banks[0x10];
    bool has_battery;
    bool has_rtc;
    bool needs_save;
    uint8_t rtc_s;
    uint8_t rtc_m;
    uint8_t rtc_h;
    uint8_t rtc_dl;
    uint8_t rtc_dh;
    bool rtc_selected;
    uint8_t rtc_reg;
    uint8_t rtc_latch;
    time_t rtc_last_time;
    uint8_t mbc6_rom_bank1;
    uint8_t mbc6_rom_bank2;
    uint8_t mbc6_ram_bank1;
    uint8_t mbc6_ram_bank2;
    uint8_t mbc7_state;
    uint16_t mbc7_buffer;
    uint16_t mbc7_output;
    bool mbc7_cs;
    bool mbc7_clk;
    bool mbc7_prev_clk;
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
    bool enabling_ime;
    uint8_t ie_register;
    uint8_t int_flags;
} cpu_context_t;

typedef struct cpu_aux CPUClass;

typedef void (*proc_fn)(CPUClass *);

typedef struct {
    uint8_t wram[0x8000];
    uint8_t hram[0x80];
    uint8_t wram_bank;
} ram_context_t;

typedef enum {
    IT_VBLANK = 1,
    IT_LCD_STAT = 2,
    IT_TIMER = 4,
    IT_SERIAL = 8,
    IT_JOYPAD = 16
} interrupt_t;

typedef struct {
    uint16_t div;
    uint8_t tima;
    uint8_t tma;
    uint8_t tac;
} timer_context_t;

typedef struct {
    uint8_t y;
    uint8_t x;
    uint8_t tile;
    uint8_t attributes;
} oam_entry_t;

typedef struct oam_line_entry {
    oam_entry_t entry;
    struct oam_line_entry *next;
} oam_line_entry_t;

typedef enum {
    FS_TILE,
    FS_DATA0,
    FS_DATA1,
    FS_IDLE,
    FS_PUSH,
} fetch_state_t;

typedef struct fifo_entry fifo_entry_t;

typedef struct fifo_entry {
    fifo_entry_t *next;
    uint32_t value;
} fifo_entry_t;

typedef struct {
    fifo_entry_t *head;
    fifo_entry_t *tail;
    uint32_t size;
} fifo_t;

typedef struct {
    fetch_state_t state;
    fifo_t pixel_fifo;
    uint8_t line_x;
    uint8_t pushed_x;
    uint8_t fetch_x;
    uint8_t bg_fetch_data[4];
    uint8_t fetch_entry_data[6];
    uint8_t map_y;
    uint8_t map_x;
    uint8_t tile_y;
    uint8_t fifo_x;
} fifo_context_t;

typedef struct {
    oam_entry_t oam_ram[OAM_ENTRIES];
    uint8_t vram[0x4000];
    uint8_t vram_bank;
    fifo_context_t *pixel_context;
    uint8_t line_sprite_count;
    oam_line_entry_t *line_sprites;
    oam_line_entry_t line_entry_array[MAX_SPRITES];
    uint8_t fetch_entry_count;
    oam_entry_t fetched_entries[3];
    uint8_t window_line;
    uint32_t current_frame;
    uint32_t line_ticks;
    uint32_t *video_buffer;
    bool window_triggered;
    bool window_rendered_this_line;
} ppu_context_t;

typedef struct {
    bool active;
    uint8_t byte;
    uint8_t value;
    uint8_t start_delay;
} dma_context_t;

typedef struct {
    bool active;
    bool hblank_mode;
    uint16_t source;
    uint16_t dest;
    uint16_t remaining;
    uint8_t hdma1;
    uint8_t hdma2;
    uint8_t hdma3;
    uint8_t hdma4;
    uint8_t hdma5;
} hdma_context_t;

typedef struct {
    uint8_t control;
    uint8_t status;
    uint8_t scroll_y;
    uint8_t scroll_x;
    uint8_t y_coord;
    uint8_t y_compare;
    uint8_t dma;
    uint8_t bg_palette;
    uint8_t sprite_palette[2];
    uint8_t window_y;
    uint8_t window_x;
    uint32_t bg_colors[4];
    uint32_t sprite1_colors[4];
    uint32_t sprite2_colors[4];
    uint8_t bg_palette_index;
    uint8_t bg_palette_data[64];
    uint8_t sprite_palette_index;
    uint8_t sprite_palette_data[64];
    uint32_t bg_colors_cgb[8][4];
    uint32_t sprite_colors_cgb[8][4];
    hdma_context_t hdma;
    uint8_t opri;
    uint8_t undoc_ff72;
    uint8_t undoc_ff73;
    uint8_t undoc_ff74;
    uint8_t undoc_ff75;
} lcd_context_t;

typedef enum {
    MODE_HBLANK,
    MODE_VBLANK,
    MODE_OAM,
    MODE_TRANSFER,
} lcd_mode_t;

typedef enum {
    SS_HBLANK = (1 << HBLANK_OFF),
    SS_VBLANK = (1 << VBLANK_OFF),
    SS_OAM = (1 << OAM_OFF),
    SS_LYC = (1 << LYC_OFF),
} stat_src_t;

typedef struct {
    bool start : 1;
    bool select : 1;
    bool a : 1;
    bool b : 1;
    bool up : 1;
    bool down : 1;
    bool left : 1;
    bool right : 1;
} joypad_state_t;

typedef struct {
    bool button_selected;
    bool direction_selected;
    joypad_state_t state;
} joypad_context_t;

typedef struct {
    bool enabled;
    uint8_t volume;
    uint8_t freq_lo;
    uint8_t freq_hi;
    uint8_t sweep_time;
    uint8_t sweep_direction;
    uint8_t sweep_shift;
    uint8_t wave_duty;
    uint8_t length;
    uint8_t initial_volume;
    uint8_t envelope_direction;
    uint8_t envelope_sweep;
    uint32_t frequency;
    uint32_t period;
    uint32_t duty_cycle;
    uint16_t lfsr;
    float_t time_counter;
    float_t envelope_counter;
    float_t sweep_counter;
} sound_channel1_t;

typedef struct {
    bool enabled;
    uint8_t volume;
    uint8_t freq_lo;
    uint8_t freq_hi;
    uint8_t wave_duty;
    uint8_t length;
    uint8_t initial_volume;
    uint8_t envelope_direction;
    uint8_t envelope_sweep;
    uint32_t frequency;
    uint32_t period;
    uint32_t duty_cycle;
    float_t time_counter;
    float_t envelope_counter;
} sound_channel2_t;

typedef struct {
    bool enabled;
    uint8_t volume;
    uint8_t freq_lo;
    uint8_t freq_hi;
    uint8_t length;
    uint8_t output_level;
    uint32_t frequency;
    uint32_t period;
    float_t time_counter;
    uint8_t wave_pattern[32];
} sound_channel3_t;

typedef struct {
    bool enabled;
    uint8_t volume;
    uint8_t length;
    uint8_t initial_volume;
    uint8_t envelope_direction;
    uint8_t envelope_sweep;
    uint8_t shift_clock_freq;
    uint8_t counter_width;
    uint8_t dividing_ratio;
    uint32_t period;
    uint16_t lfsr;
    float_t time_counter;
    float_t envelope_counter;
} sound_channel4_t;

typedef struct {
    bool initialized;
    SDL_AudioDeviceID device;
    SDL_AudioSpec spec;
    uint8_t master_volume;
    uint8_t channel_control;
    uint8_t output_select;
    uint8_t master_on;
    sound_channel1_t channel1;
    sound_channel2_t channel2;
    sound_channel3_t channel3;
    sound_channel4_t channel4;
} sound_context_t;

#endif
