#include <stdint.h>
#include "common.h"
#include "oop.h"

#ifndef __SOUND
    #define __SOUND

typedef struct gameboy_aux GameboyClass;
typedef struct sound_aux SoundClass;

typedef struct sound_aux {
    /* Properties */
    class_t metadata;
    GameboyClass *parent;
    const uint8_t duty_cycles[4];
    const uint8_t volume_levels[4];
    sound_context_t *context;
    /* Methods */
    uint8_t (*read)(SoundClass *, uint16_t);
    void (*write)(SoundClass *, uint16_t, uint8_t);
    void (*update)(SoundClass *);
    void (*audio_callback)(void *, uint8_t *, int32_t);
    void (*update_channel1)(SoundClass *, float_t);
    void (*update_channel2)(SoundClass *, float_t);
    void (*update_channel3)(SoundClass *, float_t);
    void (*update_channel4)(SoundClass *, float_t);
    float_t (*get_channel1_sample)(SoundClass *);
    float_t (*get_channel2_sample)(SoundClass *);
    float_t (*get_channel3_sample)(SoundClass *);
    float_t (*get_channel4_sample)(SoundClass *);
    void (*init_sound_system)(SoundClass *);
    void (*update_volume)(SoundClass *, bool);
} SoundClass;

extern const class_t *Sound;
#endif
