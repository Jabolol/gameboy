#include "../include/sound.h"
#include <SDL2/SDL.h>
#include <math.h>
#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    SoundClass *self = (SoundClass *) ptr;
    if (!((self->context = calloc(1, sizeof(*self->context))))) {
        HANDLE_ERROR("failed memory allocation");
    }
    self->parent = va_arg(*args, GameboyClass *);

    self->context->master_volume = 0x77;
    self->context->channel_control = 0xF3;
    self->context->master_on = 0x80;

    self->context->channel1.period = 0;
    self->context->channel1.duty_cycle = 0;
    self->context->channel1.volume = 0;
    self->context->channel1.time_counter = 0;
    self->context->channel1.envelope_counter = 0;
    self->context->channel1.sweep_counter = 0;
    self->context->channel1.lfsr = 0x7FFF;

    self->context->channel2.period = 0;
    self->context->channel2.duty_cycle = 0;
    self->context->channel2.volume = 0;
    self->context->channel2.time_counter = 0;
    self->context->channel2.envelope_counter = 0;

    self->context->channel3.period = 0;
    self->context->channel3.volume = 0;
    self->context->channel3.time_counter = 0;
    memset(self->context->channel3.wave_pattern, 0,
        sizeof(self->context->channel3.wave_pattern));

    self->context->channel4.period = 0;
    self->context->channel4.volume = 0;
    self->context->channel4.lfsr = 0x7FFF;
    self->context->channel4.time_counter = 0;
    self->context->channel4.envelope_counter = 0;

    self->init_sound_system(self);

    LOG("Sound system initialized");
}

static void destructor(void *ptr)
{
    SoundClass *self = (SoundClass *) ptr;

    if (self->context->initialized) {
        SDL_CloseAudioDevice(self->context->device);
    }

    free(self->context);
}

static float_t get_channel1_sample(SoundClass *self)
{
    sound_channel1_t *channel = &self->context->channel1;
    if (channel->volume == 0 || channel->period == 0) {
        return 0.0f;
    }

    float_t cycle_pos =
        fmodf(channel->time_counter, 1.0f / channel->period) * channel->period;

    float_t sample = 0.0f;

    switch (channel->wave_duty) {
        case 0: sample = (cycle_pos < 0.125f) ? 1.0f : -1.0f; break;
        case 1: sample = (cycle_pos < 0.25f) ? 1.0f : -1.0f; break;
        case 2: sample = (cycle_pos < 0.5f) ? 1.0f : -1.0f; break;
        case 3: sample = (cycle_pos < 0.75f) ? 1.0f : -1.0f; break;
    }

    return sample * (channel->volume / 15.0f);
}

static float_t get_channel2_sample(SoundClass *self)
{
    sound_channel2_t *channel = &self->context->channel2;
    if (channel->volume == 0 || channel->period == 0) {
        return 0.0f;
    }

    float_t cycle_pos =
        fmodf(channel->time_counter, 1.0f / channel->period) * channel->period;

    float_t sample = 0.0f;

    switch (channel->wave_duty) {
        case 0: sample = (cycle_pos < 0.125f) ? 1.0f : -1.0f; break;
        case 1: sample = (cycle_pos < 0.25f) ? 1.0f : -1.0f; break;
        case 2: sample = (cycle_pos < 0.5f) ? 1.0f : -1.0f; break;
        case 3: sample = (cycle_pos < 0.75f) ? 1.0f : -1.0f; break;
    }

    return sample * (channel->volume / 15.0f);
}

static float_t get_channel3_sample(SoundClass *self)
{
    sound_channel3_t *channel = &self->context->channel3;
    if (channel->volume == 0 || channel->period == 0) {
        return 0.0f;
    }

    float_t cycle_pos =
        fmodf(channel->time_counter, 1.0f / channel->period) * channel->period;
    int32_t pattern_pos = (int32_t) (cycle_pos * 32) % 32;

    int32_t sample_byte = pattern_pos / 2;
    int32_t sample_nibble = pattern_pos % 2;

    int32_t raw_sample = (sample_nibble == 0)
        ? ((channel->wave_pattern[sample_byte] >> 4) & 0x0F)
        : (channel->wave_pattern[sample_byte] & 0x0F);

    float_t sample = (raw_sample / 7.5f) - 1.0f;

    return sample * (channel->volume / 15.0f);
}

static float_t get_channel4_sample(SoundClass *self)
{
    sound_channel4_t *channel = &self->context->channel4;
    if (channel->volume == 0 || channel->period == 0) {
        return 0.0f;
    }

    float_t sample = (channel->lfsr & 0x1) ? 1.0f : -1.0f;

    return sample * (channel->volume / 15.0f);
}

static void init_sound_system(SoundClass *self)
{
    if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO)) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            fprintf(stderr, "SDL Audio initialization failed: %s\n",
                SDL_GetError());
            return;
        }
    }

    SDL_AudioSpec desired;
    SDL_zero(desired);
    desired.freq = AUDIO_FREQUENCY;
    desired.format = AUDIO_FORMAT;
    desired.channels = AUDIO_CHANNELS;
    desired.samples = AUDIO_SAMPLES;
    desired.callback = self->audio_callback;
    desired.userdata = self;

    self->context->device =
        SDL_OpenAudioDevice(NULL, 0, &desired, &self->context->spec, 0);
    if (self->context->device == 0) {
        fprintf(stderr, "Failed to open audio device: %s\n", SDL_GetError());
        return;
    }

    SDL_PauseAudioDevice(self->context->device, 0);
    self->context->initialized = true;
}

static uint8_t read(SoundClass *self, uint16_t address)
{
    SDL_LockAudioDevice(self->context->device);

    uint8_t value = 0xFF;

    switch (address) {
        case 0xFF10: {
            value = (self->context->channel1.sweep_time << 4)
                | (self->context->channel1.sweep_direction << 3)
                | (self->context->channel1.sweep_shift);
            break;
        }
        case 0xFF11: {
            value = (self->context->channel1.wave_duty << 6)
                | (self->context->channel1.length & 0x3F);
            break;
        }
        case 0xFF12: {
            value = (self->context->channel1.initial_volume << 4)
                | (self->context->channel1.envelope_direction << 3)
                | (self->context->channel1.envelope_sweep);
            break;
        }
        case 0xFF13: {
            value = self->context->channel1.freq_lo;
            break;
        }
        case 0xFF14: {
            value = (self->context->channel1.freq_hi & 0x07);
            break;
        }
        case 0xFF16: {
            value = (self->context->channel1.wave_duty << 6)
                | (self->context->channel1.length & 0x3F);
            break;
        }
        case 0xFF17: {
            value = (self->context->channel2.initial_volume << 4)
                | (self->context->channel2.envelope_direction << 3)
                | (self->context->channel2.envelope_sweep);
            break;
        }
        case 0xFF18: {
            value = self->context->channel2.freq_lo;
            break;
        }
        case 0xFF19: {
            value = (self->context->channel2.freq_hi & 0x07) | 0x80;
            break;
        }
        case 0xFF1A: {
            value = (self->context->channel3.enabled ? 0x80 : 0);
            break;
        }
        case 0xFF1B: {
            value = self->context->channel3.length;
            break;
        }
        case 0xFF1C: {
            value = self->context->channel3.output_level << 5;
            break;
        }
        case 0xFF1D: {
            value = self->context->channel3.freq_lo;
            break;
        }
        case 0xFF1E: {
            value = (self->context->channel3.freq_hi & 0x07) | 0x80;
            break;
        }
        case 0xFF20: {
            value = self->context->channel4.length & 0x3F;
            break;
        }
        case 0xFF21: {
            value = (self->context->channel4.initial_volume << 4)
                | (self->context->channel4.envelope_direction << 3)
                | (self->context->channel4.envelope_sweep);
            break;
        }
        case 0xFF22: {
            value = (self->context->channel4.shift_clock_freq << 4)
                | (self->context->channel4.counter_width << 3)
                | (self->context->channel4.dividing_ratio);
            break;
        }
        case 0xFF23: {
            value = 0xBF | (self->context->channel4.enabled ? 0x80 : 0);
            break;
        }
        case 0xFF24: {
            value = self->context->master_volume;
            break;
        }
        case 0xFF25: {
            value = self->context->channel_control;
            break;
        }
        case 0xFF26: {
            value = self->context->master_on
                | (self->context->channel1.enabled ? 0x01 : 0)
                | (self->context->channel2.enabled ? 0x02 : 0)
                | (self->context->channel3.enabled ? 0x04 : 0)
                | (self->context->channel4.enabled ? 0x08 : 0);
            break;
        }
        default: {
            if (address >= 0xFF30 && address <= 0xFF3F) {
                value = self->context->channel3.wave_pattern[address - 0xFF30];
            }
            break;
        }
    }

    SDL_UnlockAudioDevice(self->context->device);
    return value;
}

static void write(SoundClass *self, uint16_t address, uint8_t value)
{
    if (!(self->context->master_on & 0x80) && address != 0xFF26) {
        return;
    }

    SDL_LockAudioDevice(self->context->device);

    switch (address) {
        case 0xFF10: {
            self->context->channel1.sweep_time = (value >> 4) & 0x07;
            self->context->channel1.sweep_direction = (value >> 3) & 0x01;
            self->context->channel1.sweep_shift = value & 0x07;
            break;
        }
        case 0xFF11: {
            self->context->channel1.wave_duty =
                self->duty_cycles[(value >> 6) & 0x03];
            self->context->channel1.length = value & 0x3F;
            break;
        }
        case 0xFF12: {
            self->context->channel1.initial_volume = (value >> 4) & 0x0F;
            self->context->channel1.envelope_direction = (value >> 3) & 0x01;
            self->context->channel1.envelope_sweep = value & 0x07;

            self->context->channel1.volume =
                self->context->channel1.initial_volume;
            self->context->channel1.envelope_counter = 0;

            if (self->context->channel1.initial_volume == 0
                && self->context->channel1.envelope_direction == 0) {
                self->context->channel1.enabled = false;
            }
            break;
        }
        case 0xFF13: {
            self->context->channel1.freq_lo = value;
            self->context->channel1.frequency =
                ((self->context->channel1.freq_hi & 0x07) << 8) | value;
            self->context->channel1.period =
                131072 / (2048 - self->context->channel1.frequency);
            break;
        }
        case 0xFF14: {
            bool trigger = (value >> 7) & 0x01;
            self->context->channel1.freq_hi = value & 0x07;

            self->context->channel1.frequency =
                ((self->context->channel1.freq_hi & 0x07) << 8)
                | self->context->channel1.freq_lo;
            self->context->channel1.period =
                131072 / (2048 - self->context->channel1.frequency);

            if (trigger) {
                self->context->channel1.enabled = true;
                self->context->channel1.volume =
                    self->context->channel1.initial_volume;
                self->context->channel1.time_counter = 0;
                self->context->channel1.envelope_counter = 0;
                self->context->channel1.sweep_counter = 0;
            }
            break;
        }
        case 0xFF16: {
            self->context->channel2.wave_duty =
                self->duty_cycles[(value >> 6) & 0x03];
            self->context->channel2.length = value & 0x3F;
            break;
        }
        case 0xFF17: {
            self->context->channel2.initial_volume = (value >> 4) & 0x0F;
            self->context->channel2.envelope_direction = (value >> 3) & 0x01;
            self->context->channel2.envelope_sweep = value & 0x07;

            self->context->channel2.volume =
                self->context->channel2.initial_volume;
            self->context->channel2.envelope_counter = 0;

            if (self->context->channel2.initial_volume == 0
                && self->context->channel2.envelope_direction == 0) {
                self->context->channel2.enabled = false;
            }
            break;
        }
        case 0xFF18: {
            self->context->channel2.freq_lo = value;
            self->context->channel2.frequency =
                ((self->context->channel2.freq_hi & 0x07) << 8) | value;
            self->context->channel2.period =
                131072 / (2048 - self->context->channel2.frequency);
            break;
        }
        case 0xFF19: {
            bool trigger = (value >> 7) & 0x01;
            self->context->channel2.freq_hi = value & 0x07;

            self->context->channel2.frequency =
                ((self->context->channel2.freq_hi & 0x07) << 8)
                | self->context->channel2.freq_lo;
            self->context->channel2.period =
                131072 / (2048 - self->context->channel2.frequency);

            if (trigger) {
                self->context->channel2.enabled = true;
                self->context->channel2.volume =
                    self->context->channel2.initial_volume;
                self->context->channel2.time_counter = 0;
                self->context->channel2.envelope_counter = 0;
            }
            break;
        }
        case 0xFF1A: {
            self->context->channel3.enabled = (value & 0x80) != 0;
            break;
        }
        case 0xFF1B: {
            self->context->channel3.length = value;
            break;
        }
        case 0xFF1C: {
            self->context->channel3.output_level = (value >> 5) & 0x03;

            self->context->channel3.volume =
                self->volume_levels[self->context->channel3.output_level];
            break;
        }
        case 0xFF1D: {
            self->context->channel3.freq_lo = value;
            self->context->channel3.frequency =
                ((self->context->channel3.freq_hi & 0x07) << 8) | value;
            self->context->channel3.period =
                131072 / (2048 - self->context->channel3.frequency) / 2;
            break;
        }
        case 0xFF1E: {
            bool trigger = (value >> 7) & 0x01;
            self->context->channel3.freq_hi = value & 0x07;

            self->context->channel3.frequency =
                ((self->context->channel3.freq_hi & 0x07) << 8)
                | self->context->channel3.freq_lo;
            self->context->channel3.period =
                131072 / (2048 - self->context->channel3.frequency) / 2;

            if (trigger) {
                self->context->channel3.enabled = true;
                self->context->channel3.time_counter = 0;
            }
            break;
        }
        case 0xFF20: {
            self->context->channel4.length = value & 0x3F;
            break;
        }
        case 0xFF21: {
            self->context->channel4.initial_volume = (value >> 4) & 0x0F;
            self->context->channel4.envelope_direction = (value >> 3) & 0x01;
            self->context->channel4.envelope_sweep = value & 0x07;

            self->context->channel4.volume =
                self->context->channel4.initial_volume;
            self->context->channel4.envelope_counter = 0;

            if (self->context->channel4.initial_volume == 0
                && self->context->channel4.envelope_direction == 0) {
                self->context->channel4.enabled = false;
            }
            break;
        }
        case 0xFF22: {
            self->context->channel4.shift_clock_freq = (value >> 4) & 0x0F;
            self->context->channel4.counter_width = (value >> 3) & 0x01;
            self->context->channel4.dividing_ratio = value & 0x07;

            int32_t dividing_ratio =
                (self->context->channel4.dividing_ratio == 0)
                ? 8
                : self->context->channel4.dividing_ratio * 16;
            self->context->channel4.period = dividing_ratio
                << self->context->channel4.shift_clock_freq;
            break;
        }
        case 0xFF23: {
            bool trigger = (value >> 7) & 0x01;

            if (trigger) {
                self->context->channel4.enabled = true;
                self->context->channel4.volume =
                    self->context->channel4.initial_volume;
                self->context->channel4.time_counter = 0;
                self->context->channel4.envelope_counter = 0;
                self->context->channel4.lfsr = 0x7FFF;
            }
            break;
        }
        case 0xFF24: {
            self->context->master_volume = value;
            break;
        }
        case 0xFF25: {
            self->context->channel_control = value;
            break;
        }
        case 0xFF26: {
            bool new_master = (value & 0x80) != 0;
            bool old_master = (self->context->master_on & 0x80) != 0;

            self->context->master_on = value & 0x80;

            if (old_master && !new_master) {
                memset(&self->context->channel1, 0,
                    sizeof(self->context->channel1));
                memset(&self->context->channel2, 0,
                    sizeof(self->context->channel2));
                memset(&self->context->channel3, 0,
                    sizeof(self->context->channel3));
                memset(&self->context->channel4, 0,
                    sizeof(self->context->channel4));
                self->context->master_volume = 0;
                self->context->channel_control = 0;
            }
            break;
        }
        default: {
            if (address >= 0xFF30 && address <= 0xFF3F) {
                self->context->channel3.wave_pattern[address - 0xFF30] = value;
            }
            break;
        }
    }

    SDL_UnlockAudioDevice(self->context->device);
}

static void update(SoundClass *self)
{
    if (!(self->context->master_on & 0x80)) {
        return;
    }
}

static void audio_callback(void *userdata, uint8_t *stream, int32_t len)
{
    SoundClass *self = (SoundClass *) userdata;
    int16_t *buffer = (int16_t *) stream;
    int32_t sample_count = len / 4;

    if (!(self->context->master_on & 0x80)) {
        memset(stream, 0, len);
        return;
    }

    float_t dt = 1.0f / AUDIO_FREQUENCY;

    for (int32_t i = 0; i < sample_count; i++) {
        float_t left_sample = 0;
        float_t right_sample = 0;

        if (self->context->channel1.enabled) {
            self->update_channel1(self, dt);
            float_t sample = self->get_channel1_sample(self);

            if (self->context->channel_control & 0x01) {
                left_sample += sample;
            }
            if (self->context->channel_control & 0x10) {
                right_sample += sample;
            }
        }

        if (self->context->channel2.enabled) {
            self->update_channel2(self, dt);
            float_t sample = self->get_channel2_sample(self);

            if (self->context->channel_control & 0x02) {
                left_sample += sample;
            }
            if (self->context->channel_control & 0x20) {
                right_sample += sample;
            }
        }

        if (self->context->channel3.enabled) {
            self->update_channel3(self, dt);
            float_t sample = self->get_channel3_sample(self);

            if (self->context->channel_control & 0x04) {
                left_sample += sample;
            }
            if (self->context->channel_control & 0x40) {
                right_sample += sample;
            }
        }

        if (self->context->channel4.enabled) {
            self->update_channel4(self, dt);
            float_t sample = self->get_channel4_sample(self);

            if (self->context->channel_control & 0x08) {
                left_sample += sample;
            }
            if (self->context->channel_control & 0x80) {
                right_sample += sample;
            }
        }

        float_t left_vol = ((self->context->master_volume >> 4) & 0x07) / 7.0f;
        float_t right_vol = (self->context->master_volume & 0x07) / 7.0f;

        left_sample *= left_vol;
        right_sample *= right_vol;

        int16_t left_out =
            (int16_t) (fmaxf(-1.0f, fminf(1.0f, left_sample)) * 32767.0f);
        int16_t right_out =
            (int16_t) (fmaxf(-1.0f, fminf(1.0f, right_sample)) * 32767.0f);

        buffer[i * 2] = left_out;
        buffer[i * 2 + 1] = right_out;
    }
}

static void update_channel1(SoundClass *self, float_t dt)
{
    sound_channel1_t *channel = &self->context->channel1;
    channel->time_counter += dt;

    if (channel->envelope_sweep > 0) {
        channel->envelope_counter += dt;
        float_t envelope_period = channel->envelope_sweep / 64.0f;

        if (channel->envelope_counter >= envelope_period) {
            channel->envelope_counter -= envelope_period;

            if (channel->envelope_direction) {
                if (channel->volume < 15) {
                    channel->volume++;
                }
            } else {
                if (channel->volume > 0) {
                    channel->volume--;
                }
            }
        }
    }

    if (channel->sweep_time > 0 && channel->sweep_shift > 0) {
        channel->sweep_counter += dt;
        float_t sweep_period = channel->sweep_time / 128.0f;

        if (channel->sweep_counter >= sweep_period) {
            channel->sweep_counter -= sweep_period;

            uint16_t offset = channel->frequency >> channel->sweep_shift;

            if (channel->sweep_direction) {
                if (channel->frequency >= offset) {
                    channel->frequency -= offset;
                } else {
                    channel->enabled = false;
                }
            } else {
                channel->frequency += offset;

                if (channel->frequency > 2047) {
                    channel->enabled = false;
                }
            }

            channel->period = 131072 / (2048 - channel->frequency);
        }
    }
}

static void update_channel2(SoundClass *self, float_t dt)
{
    sound_channel2_t *channel = &self->context->channel2;
    channel->time_counter += dt;

    if (channel->envelope_sweep > 0) {
        channel->envelope_counter += dt;
        float_t envelope_period = channel->envelope_sweep / 64.0f;

        if (channel->envelope_counter >= envelope_period) {
            channel->envelope_counter -= envelope_period;

            if (channel->envelope_direction) {
                if (channel->volume < 15) {
                    channel->volume++;
                }
            } else {
                if (channel->volume > 0) {
                    channel->volume--;
                }
            }
        }
    }
}

static void update_channel3(SoundClass *self, float_t dt)
{
    sound_channel3_t *channel = &self->context->channel3;
    channel->time_counter += dt;
}

static void update_channel4(SoundClass *self, float_t dt)
{
    sound_channel4_t *channel = &self->context->channel4;
    channel->time_counter += dt;

    if (channel->envelope_sweep > 0) {
        channel->envelope_counter += dt;
        float_t envelope_period = channel->envelope_sweep / 64.0f;

        if (channel->envelope_counter >= envelope_period) {
            channel->envelope_counter -= envelope_period;

            if (channel->envelope_direction) {
                if (channel->volume < 15) {
                    channel->volume++;
                }
            } else {
                if (channel->volume > 0) {
                    channel->volume--;
                }
            }
        }
    }

    if (channel->period == 0) {
        return;
    }

    int32_t noise_freq = 524288 / channel->period;
    float_t steps = dt * noise_freq;

    for (int32_t i = 0; i < steps; i++) {
        uint8_t bit = (channel->lfsr & 0x1) ^ ((channel->lfsr >> 1) & 0x1);

        channel->lfsr >>= 1;
        channel->lfsr |= (bit << 14);

        if (channel->counter_width) {
            channel->lfsr &= ~(1 << 6);
            channel->lfsr |= (bit << 6);
        }
    }
}

static void update_volume(SoundClass *self, bool up)
{
    if (up) {
        if (self->context->master_volume < 0x77) {
            self->context->master_volume++;
        }
    } else {
        if (self->context->master_volume > 0) {
            self->context->master_volume--;
        }
    }
}

const SoundClass init_sound = {
    {
        ._size = sizeof(SoundClass),
        ._name = "Sound",
        ._constructor = constructor,
        ._destructor = destructor,
    },
    .duty_cycles = {0, 1, 2, 3},
    .volume_levels = {0, 15, 7, 3},
    .read = read,
    .write = write,
    .update = update,
    .audio_callback = audio_callback,
    .update_channel1 = update_channel1,
    .update_channel2 = update_channel2,
    .update_channel3 = update_channel3,
    .update_channel4 = update_channel4,
    .get_channel1_sample = get_channel1_sample,
    .get_channel2_sample = get_channel2_sample,
    .get_channel3_sample = get_channel3_sample,
    .get_channel4_sample = get_channel4_sample,
    .init_sound_system = init_sound_system,
    .update_volume = update_volume,
};

const class_t *Sound = (const class_t *) &init_sound;
