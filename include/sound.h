#include <stdint.h>
#include "common.h"
#include "oop.h"

#ifndef __SOUND
    #define __SOUND

typedef struct gameboy_aux GameboyClass;
typedef struct sound_aux SoundClass;
typedef struct snapshot_aux SnapshotClass;

typedef struct sound_aux {
    /* Properties */
    class_t metadata;
    GameboyClass *parent;
    const uint8_t duty_cycles[4];
    const uint8_t volume_shifts[4];
    const uint8_t read_masks[0x17];
    const uint8_t noise_divisors[8];
    sound_context_t *context;
    /* Methods */
    uint8_t (*read)(SoundClass *, uint16_t);
    void (*write)(SoundClass *, uint16_t, uint8_t);
    void (*tick)(SoundClass *, uint32_t);
    void (*step_pulse)(SoundClass *, sound_channel_t *, uint32_t);
    void (*step_wave)(SoundClass *, sound_channel_t *, uint32_t);
    void (*step_noise)(SoundClass *, sound_channel_t *, uint32_t);
    void (*sequencer_step)(SoundClass *);
    void (*reset)(SoundClass *);
    void (*power_off)(SoundClass *);
    void (*trigger)(SoundClass *, uint8_t);
    uint16_t (*sweep_calculate)(SoundClass *);
    void (*clock_length)(SoundClass *);
    void (*clock_sweep)(SoundClass *);
    void (*clock_envelope)(SoundClass *);
    void (*write_length_enable)(SoundClass *, uint8_t, uint8_t);
    uint8_t (*channel_output)(SoundClass *, uint8_t);
    uint8_t (*pcm)(SoundClass *, bool);
    void (*emit_sample)(SoundClass *);
    void (*mix)(SoundClass *);
    void (*audio_callback)(void *, uint8_t *, int32_t);
    void (*init_sound_system)(SoundClass *);
    void (*update_volume)(SoundClass *, bool);
    void (*serialize)(SoundClass *, SnapshotClass *);
} SoundClass;

extern const class_t *Sound;
#endif
