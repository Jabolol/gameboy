#include "../include/sound.h"
#include <SDL2/SDL.h>
#include <math.h>
#include "../include/gameboy.h"

#define NR10 0x00
#define NR11 0x01
#define NR12 0x02
#define NR13 0x03
#define NR14 0x04
#define NR21 0x06
#define NR22 0x07
#define NR23 0x08
#define NR24 0x09
#define NR30 0x0A
#define NR31 0x0B
#define NR32 0x0C
#define NR33 0x0D
#define NR34 0x0E
#define NR41 0x10
#define NR42 0x11
#define NR43 0x12
#define NR44 0x13
#define NR50 0x14
#define NR51 0x15
#define NR52 0x16

static void constructor(void *ptr, va_list *args)
{
    SoundClass *self = (SoundClass *) ptr;
    if (!((self->context = calloc(1, sizeof(*self->context))))) {
        HANDLE_ERROR("failed memory allocation");
    }
    self->parent = va_arg(*args, GameboyClass *);
    atomic_init(&self->context->output_volume, 1.0f);
    atomic_init(&self->context->ring.read_index, 0);
    atomic_init(&self->context->ring.write_index, 0);

    self->init_sound_system(self);
    self->reset(self);

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

static void init_sound_system(SoundClass *self)
{
    sound_context_t *ctx = self->context;

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

    ctx->device = SDL_OpenAudioDevice(NULL, 0, &desired, &ctx->spec, 0);
    if (ctx->device == 0) {
        fprintf(stderr, "Failed to open audio device: %s\n", SDL_GetError());
        return;
    }

    ctx->ticks_per_sample = (double) CLOCK_SPEED / ctx->spec.freq;
    ctx->hpf_charge = powf(0.999958f, (float) ctx->ticks_per_sample);
    ctx->initialized = true;
    SDL_PauseAudioDevice(ctx->device, 0);
}

static void power_off(SoundClass *self)
{
    sound_context_t *ctx = self->context;
    bool keep_length = self->parent->context->hw_mode == HW_DMG;

    memset(ctx->registers, 0, NR52);
    for (uint8_t i = 0; i < 4; i++) {
        uint16_t length = ctx->channels[i].length_counter;
        memset(&ctx->channels[i], 0, sizeof(ctx->channels[i]));
        ctx->channels[i].length_counter = keep_length ? length : 0;
    }
    ctx->power = false;
}

static void reset(SoundClass *self)
{
    static const uint8_t wave_ram[16] = {0x84, 0x40, 0x43, 0xAA, 0x2D, 0x78,
        0x92, 0x3C, 0x60, 0x59, 0x59, 0xB0, 0x34, 0xB8, 0x2E, 0xDA};
    static const uint8_t boot_values[][2] = {{NR10, 0x80}, {NR11, 0xBF},
        {NR12, 0xF3}, {NR13, 0xFF}, {NR21, 0x3F}, {NR22, 0x00}, {NR23, 0xFF},
        {NR30, 0x7F}, {NR31, 0xFF}, {NR32, 0x9F}, {NR33, 0xFF}, {NR41, 0xFF},
        {NR42, 0x00}, {NR43, 0x00}, {NR50, 0x77}, {NR51, 0xF3}};
    sound_context_t *ctx = self->context;

    self->power_off(self);
    memcpy(ctx->wave_ram, wave_ram, sizeof(ctx->wave_ram));
    self->write(self, 0xFF10 + NR52, 0x80);
    for (uint8_t i = 0; i < sizeof(boot_values) / sizeof(*boot_values); i++) {
        self->write(self, 0xFF10 + boot_values[i][0], boot_values[i][1]);
    }
    ctx->channels[0].enabled = true;
}

static uint8_t channel_output(SoundClass *self, uint8_t index)
{
    sound_channel_t *channel = &self->context->channels[index];

    if (!channel->enabled) {
        return 0;
    }
    switch (index) {
        case 0:
        case 1: {
            bool high = (self->duty_cycles[channel->duty]
                            >> (7 - channel->duty_position))
                & 1;
            return high ? channel->volume : 0;
        }
        case 2:
            return channel->wave_sample
                >> self->volume_shifts[channel->output_level];
        default: return (~channel->lfsr & 1) ? channel->volume : 0;
    }
}

static uint8_t pcm(SoundClass *self, bool high_channels)
{
    uint8_t first = high_channels ? 2 : 0;
    return self->channel_output(self, first)
        | (self->channel_output(self, first + 1) << 4);
}

static uint16_t sweep_calculate(SoundClass *self)
{
    sound_channel_t *channel = &self->context->channels[0];
    uint16_t delta = channel->sweep_shadow >> channel->sweep_shift;
    uint16_t frequency;

    if (channel->sweep_negate) {
        frequency = channel->sweep_shadow - delta;
        channel->sweep_negated = true;
    } else {
        frequency = channel->sweep_shadow + delta;
    }
    if (frequency > 2047) {
        channel->enabled = false;
    }
    return frequency;
}

static void clock_length(SoundClass *self)
{
    for (uint8_t i = 0; i < 4; i++) {
        sound_channel_t *channel = &self->context->channels[i];
        if (channel->length_enabled && channel->length_counter > 0) {
            if (--channel->length_counter == 0) {
                channel->enabled = false;
            }
        }
    }
}

static void clock_sweep(SoundClass *self)
{
    sound_channel_t *channel = &self->context->channels[0];

    if (channel->sweep_timer > 0 && --channel->sweep_timer > 0) {
        return;
    }
    channel->sweep_timer = channel->sweep_period ? channel->sweep_period : 8;
    if (!channel->sweep_enabled || !channel->sweep_period) {
        return;
    }

    uint16_t frequency = self->sweep_calculate(self);
    if (frequency <= 2047 && channel->sweep_shift) {
        channel->frequency = frequency;
        channel->sweep_shadow = frequency;
        self->context->registers[NR13] = frequency & 0xFF;
        self->context->registers[NR14] =
            (self->context->registers[NR14] & ~0x07) | (frequency >> 8);
        self->sweep_calculate(self);
    }
}

static void clock_envelope(SoundClass *self)
{
    static const uint8_t channels[] = {0, 1, 3};

    for (uint8_t i = 0; i < 3; i++) {
        sound_channel_t *channel = &self->context->channels[channels[i]];
        if (!channel->envelope_period) {
            continue;
        }
        if (channel->envelope_timer > 0 && --channel->envelope_timer > 0) {
            continue;
        }
        channel->envelope_timer = channel->envelope_period;
        if (channel->envelope_add && channel->volume < 15) {
            channel->volume++;
        } else if (!channel->envelope_add && channel->volume > 0) {
            channel->volume--;
        }
    }
}

static void sequencer_step(SoundClass *self)
{
    sound_context_t *ctx = self->context;

    if (!ctx->power) {
        return;
    }
    switch (ctx->sequencer_step) {
        case 0:
        case 4: self->clock_length(self); break;
        case 2:
        case 6: {
            self->clock_length(self);
            self->clock_sweep(self);
            break;
        }
        case 7: self->clock_envelope(self); break;
    }
    ctx->sequencer_step = (ctx->sequencer_step + 1) & 7;
    ctx->mix_dirty = true;
}

static void trigger(SoundClass *self, uint8_t index)
{
    sound_context_t *ctx = self->context;
    sound_channel_t *channel = &ctx->channels[index];
    uint16_t max_length = index == 2 ? 256 : 64;
    bool was_enabled = channel->enabled;

    channel->enabled = channel->dac_enabled;
    if (channel->length_counter == 0) {
        channel->length_counter = max_length;
        if (channel->length_enabled && (ctx->sequencer_step & 1)) {
            channel->length_counter--;
        }
    }

    switch (index) {
        case 0:
        case 1: channel->timer = (2048 - channel->frequency) * 4; break;
        case 2: {
            if (was_enabled && channel->timer == 2
                && self->parent->context->hw_mode == HW_DMG) {
                uint8_t offset = ((channel->wave_position + 1) >> 1) & 0x0F;
                if (offset < 4) {
                    ctx->wave_ram[0] = ctx->wave_ram[offset];
                } else {
                    memmove(ctx->wave_ram, ctx->wave_ram + (offset & ~3), 4);
                }
            }
            channel->timer = (2048 - channel->frequency) * 2 + 6;
            channel->wave_position = 0;
            channel->wave_just_read = false;
            break;
        }
        case 3: {
            channel->timer = self->noise_divisors[channel->divisor_code]
                << channel->clock_shift;
            channel->lfsr = 0x7FFF;
            break;
        }
    }

    if (index != 2) {
        channel->volume = channel->envelope_initial;
        channel->envelope_timer =
            channel->envelope_period ? channel->envelope_period : 8;
    }

    if (index == 0) {
        channel->sweep_shadow = channel->frequency;
        channel->sweep_timer =
            channel->sweep_period ? channel->sweep_period : 8;
        channel->sweep_enabled = channel->sweep_period || channel->sweep_shift;
        channel->sweep_negated = false;
        if (channel->sweep_shift) {
            self->sweep_calculate(self);
        }
    }
}

static void write_length_enable(SoundClass *self, uint8_t index, uint8_t value)
{
    sound_context_t *ctx = self->context;
    sound_channel_t *channel = &ctx->channels[index];
    bool was_enabled = channel->length_enabled;

    channel->length_enabled = value & 0x40;
    if (!was_enabled && channel->length_enabled && (ctx->sequencer_step & 1)
        && channel->length_counter > 0) {
        if (--channel->length_counter == 0 && !(value & 0x80)) {
            channel->enabled = false;
        }
    }
    if (value & 0x80) {
        self->trigger(self, index);
    }
}

static uint8_t read(SoundClass *self, uint16_t address)
{
    sound_context_t *ctx = self->context;
    uint8_t reg = address - 0xFF10;

    if (address >= 0xFF30) {
        sound_channel_t *wave = &ctx->channels[2];
        if (!wave->enabled) {
            return ctx->wave_ram[address - 0xFF30];
        }
        if (self->parent->context->hw_mode == HW_DMG
            && !wave->wave_just_read) {
            return 0xFF;
        }
        return ctx->wave_ram[wave->wave_position >> 1];
    }
    if (reg == NR52) {
        return (ctx->power ? 0x80 : 0x00) | 0x70
            | (ctx->channels[0].enabled ? 0x01 : 0)
            | (ctx->channels[1].enabled ? 0x02 : 0)
            | (ctx->channels[2].enabled ? 0x04 : 0)
            | (ctx->channels[3].enabled ? 0x08 : 0);
    }
    if (reg > NR52) {
        return 0xFF;
    }
    return ctx->registers[reg] | self->read_masks[reg];
}

static void write(SoundClass *self, uint16_t address, uint8_t value)
{
    sound_context_t *ctx = self->context;
    uint8_t reg = address - 0xFF10;
    sound_channel_t *channels = ctx->channels;

    ctx->mix_dirty = true;
    if (address >= 0xFF30) {
        sound_channel_t *wave = &channels[2];
        if (!wave->enabled) {
            ctx->wave_ram[address - 0xFF30] = value;
        } else if (self->parent->context->hw_mode == HW_CGB
            || wave->wave_just_read) {
            ctx->wave_ram[wave->wave_position >> 1] = value;
        }
        return;
    }
    if (reg > NR52) {
        return;
    }
    if (reg == NR52) {
        bool power = value & 0x80;
        if (ctx->power && !power) {
            self->power_off(self);
        } else if (!ctx->power && power) {
            ctx->power = true;
            ctx->sequencer_step = 0;
            for (uint8_t i = 0; i < 4; i++) {
                channels[i].duty_position = 0;
            }
            channels[2].wave_sample = 0;
        }
        return;
    }
    if (!ctx->power) {
        if (self->parent->context->hw_mode == HW_DMG) {
            switch (reg) {
                case NR11:
                    channels[0].length_counter = 64 - (value & 0x3F);
                    break;
                case NR21:
                    channels[1].length_counter = 64 - (value & 0x3F);
                    break;
                case NR31: channels[2].length_counter = 256 - value; break;
                case NR41:
                    channels[3].length_counter = 64 - (value & 0x3F);
                    break;
            }
        }
        return;
    }

    ctx->registers[reg] = value;

    switch (reg) {
        case NR10: {
            channels[0].sweep_period = (value >> 4) & 0x07;
            bool negate = value & 0x08;
            if (channels[0].sweep_negate && !negate
                && channels[0].sweep_negated) {
                channels[0].enabled = false;
            }
            channels[0].sweep_negate = negate;
            channels[0].sweep_shift = value & 0x07;
            break;
        }
        case NR11:
        case NR21: {
            sound_channel_t *channel = &channels[reg == NR11 ? 0 : 1];
            channel->duty = (value >> 6) & 0x03;
            channel->length_counter = 64 - (value & 0x3F);
            break;
        }
        case NR12:
        case NR22:
        case NR42: {
            sound_channel_t *channel =
                &channels[reg == NR12 ? 0 : (reg == NR22 ? 1 : 3)];
            channel->envelope_initial = value >> 4;
            channel->envelope_add = value & 0x08;
            channel->envelope_period = value & 0x07;
            channel->dac_enabled = value & 0xF8;
            if (!channel->dac_enabled) {
                channel->enabled = false;
            }
            break;
        }
        case NR13:
        case NR23:
        case NR33: {
            sound_channel_t *channel =
                &channels[reg == NR13 ? 0 : (reg == NR23 ? 1 : 2)];
            channel->frequency = (channel->frequency & 0x700) | value;
            break;
        }
        case NR14:
        case NR24:
        case NR34: {
            uint8_t index = reg == NR14 ? 0 : (reg == NR24 ? 1 : 2);
            channels[index].frequency =
                (channels[index].frequency & 0xFF) | ((value & 0x07) << 8);
            self->write_length_enable(self, index, value);
            break;
        }
        case NR30: {
            channels[2].dac_enabled = value & 0x80;
            if (!channels[2].dac_enabled) {
                channels[2].enabled = false;
            }
            break;
        }
        case NR31: channels[2].length_counter = 256 - value; break;
        case NR32: channels[2].output_level = (value >> 5) & 0x03; break;
        case NR41: channels[3].length_counter = 64 - (value & 0x3F); break;
        case NR43: {
            channels[3].clock_shift = value >> 4;
            channels[3].width_mode = value & 0x08;
            channels[3].divisor_code = value & 0x07;
            break;
        }
        case NR44: self->write_length_enable(self, 3, value); break;
        default: break;
    }
}

static void emit_sample(SoundClass *self)
{
    sound_context_t *ctx = self->context;
    audio_ring_t *ring = &ctx->ring;
    float left = 0.0f;
    float right = 0.0f;

    if (ctx->accumulated_ticks) {
        left = ctx->left_accumulator / ctx->accumulated_ticks;
        right = ctx->right_accumulator / ctx->accumulated_ticks;
    }
    ctx->left_accumulator = 0;
    ctx->right_accumulator = 0;
    ctx->accumulated_ticks = 0;

    float left_out = left - ctx->left_capacitor;
    float right_out = right - ctx->right_capacitor;
    ctx->left_capacitor = left - left_out * ctx->hpf_charge;
    ctx->right_capacitor = right - right_out * ctx->hpf_charge;

    float volume =
        atomic_load_explicit(&ctx->output_volume, memory_order_relaxed);
    left_out = fmaxf(-1.0f, fminf(1.0f, left_out * volume));
    right_out = fmaxf(-1.0f, fminf(1.0f, right_out * volume));

    uint32_t write_index =
        atomic_load_explicit(&ring->write_index, memory_order_relaxed);
    uint32_t read_index =
        atomic_load_explicit(&ring->read_index, memory_order_acquire);
    uint32_t fill = write_index - read_index;

    double error = ((double) fill - AUDIO_TARGET_FILL) / AUDIO_TARGET_FILL;
    error = error > 1.0 ? 1.0 : (error < -1.0 ? -1.0 : error);
    ctx->ticks_per_sample =
        ((double) CLOCK_SPEED / ctx->spec.freq) * (1.0 + error * 0.005);

    if (fill >= AUDIO_RING_FRAMES) {
        return;
    }
    uint32_t slot = (write_index % AUDIO_RING_FRAMES) * AUDIO_CHANNELS;
    ring->frames[slot] = (int16_t) (left_out * 32767.0f);
    ring->frames[slot + 1] = (int16_t) (right_out * 32767.0f);
    atomic_store_explicit(
        &ring->write_index, write_index + 1, memory_order_release);
}

static void mix(SoundClass *self)
{
    sound_context_t *ctx = self->context;
    float left = 0.0f;
    float right = 0.0f;
    uint8_t panning = ctx->registers[NR51];

    for (uint8_t i = 0; i < 4; i++) {
        if (!ctx->channels[i].dac_enabled) {
            continue;
        }
        float analog = 1.0f - self->channel_output(self, i) / 7.5f;
        if (panning & (0x10 << i)) {
            left += analog;
        }
        if (panning & (0x01 << i)) {
            right += analog;
        }
    }

    uint8_t master = ctx->registers[NR50];
    ctx->mix_left = left * ((((master >> 4) & 0x07) + 1) / 32.0f);
    ctx->mix_right = right * (((master & 0x07) + 1) / 32.0f);
    ctx->mix_dirty = false;
}

static void step_pulse(
    SoundClass *self, sound_channel_t *channel, uint32_t dots)
{
    channel->timer -= dots;
    while (channel->timer <= 0) {
        channel->timer += (2048 - channel->frequency) * 4;
        channel->duty_position = (channel->duty_position + 1) & 7;
        self->context->mix_dirty = true;
    }
}

static void step_wave(
    SoundClass *self, sound_channel_t *channel, uint32_t dots)
{
    int32_t period = (2048 - channel->frequency) * 2;

    channel->timer -= dots;
    channel->wave_just_read = false;
    while (channel->timer <= 0) {
        channel->timer += period;
        channel->wave_position = (channel->wave_position + 1) & 31;
        uint8_t byte = self->context->wave_ram[channel->wave_position >> 1];
        channel->wave_sample =
            (channel->wave_position & 1) ? (byte & 0x0F) : (byte >> 4);
        channel->wave_just_read = channel->timer == period;
        self->context->mix_dirty = true;
    }
}

static void step_noise(
    SoundClass *self, sound_channel_t *channel, uint32_t dots)
{
    if (channel->clock_shift >= 14) {
        return;
    }
    channel->timer -= dots;
    while (channel->timer <= 0) {
        channel->timer += self->noise_divisors[channel->divisor_code]
            << channel->clock_shift;
        uint16_t feedback = (channel->lfsr ^ (channel->lfsr >> 1)) & 1;
        channel->lfsr = (channel->lfsr >> 1) | (feedback << 14);
        if (channel->width_mode) {
            channel->lfsr = (channel->lfsr & ~0x40) | (feedback << 6);
        }
        self->context->mix_dirty = true;
    }
}

static void tick(SoundClass *self, uint32_t dots)
{
    sound_context_t *ctx = self->context;
    sound_channel_t *channels = ctx->channels;

    if (channels[0].enabled) {
        self->step_pulse(self, &channels[0], dots);
    }
    if (channels[1].enabled) {
        self->step_pulse(self, &channels[1], dots);
    }
    if (channels[2].enabled) {
        self->step_wave(self, &channels[2], dots);
    }
    if (channels[3].enabled) {
        self->step_noise(self, &channels[3], dots);
    }
    if (!ctx->initialized) {
        return;
    }
    if (ctx->mix_dirty) {
        self->mix(self);
    }
    ctx->left_accumulator += ctx->mix_left * dots;
    ctx->right_accumulator += ctx->mix_right * dots;
    ctx->accumulated_ticks += dots;
    ctx->sample_counter += dots;
    if (ctx->sample_counter >= ctx->ticks_per_sample) {
        ctx->sample_counter -= ctx->ticks_per_sample;
        self->emit_sample(self);
    }
}

static void audio_callback(void *userdata, uint8_t *stream, int32_t len)
{
    SoundClass *self = (SoundClass *) userdata;
    audio_ring_t *ring = &self->context->ring;
    int16_t *buffer = (int16_t *) stream;
    uint32_t frames = len / (sizeof(int16_t) * AUDIO_CHANNELS);
    uint32_t read_index =
        atomic_load_explicit(&ring->read_index, memory_order_relaxed);
    uint32_t write_index =
        atomic_load_explicit(&ring->write_index, memory_order_acquire);
    uint32_t available = write_index - read_index;
    uint32_t count = available < frames ? available : frames;

    for (uint32_t i = 0; i < count; i++) {
        uint32_t slot =
            ((read_index + i) % AUDIO_RING_FRAMES) * AUDIO_CHANNELS;
        ring->last_left = ring->frames[slot];
        ring->last_right = ring->frames[slot + 1];
        buffer[i * 2] = ring->last_left;
        buffer[i * 2 + 1] = ring->last_right;
    }

    for (uint32_t i = count; i < frames; i++) {
        buffer[i * 2] = ring->last_left;
        buffer[i * 2 + 1] = ring->last_right;
    }
    atomic_store_explicit(
        &ring->read_index, read_index + count, memory_order_release);
}

static void update_volume(SoundClass *self, bool up)
{
    float volume = atomic_load(&self->context->output_volume);

    volume += up ? 0.1f : -0.1f;
    volume = volume < 0.0f ? 0.0f : (volume > 1.0f ? 1.0f : volume);
    atomic_store(&self->context->output_volume, volume);
}

static void serialize(SoundClass *self, SnapshotClass *snapshot)
{
    sound_context_t *ctx = self->context;

    snapshot->field(snapshot, ctx->registers, sizeof(ctx->registers));
    snapshot->field(snapshot, ctx->wave_ram, sizeof(ctx->wave_ram));
    snapshot->field(snapshot, &ctx->power, sizeof(ctx->power));
    snapshot->field(
        snapshot, &ctx->sequencer_step, sizeof(ctx->sequencer_step));
    snapshot->field(snapshot, ctx->channels, sizeof(ctx->channels));
    ctx->mix_dirty = true;
}

const SoundClass init_sound = {
    {
        ._size = sizeof(SoundClass),
        ._name = "Sound",
        ._constructor = constructor,
        ._destructor = destructor,
    },
    .duty_cycles = {0b00000001, 0b10000001, 0b10000111, 0b01111110},
    .volume_shifts = {4, 0, 1, 2},
    .read_masks = {0x80, 0x3F, 0x00, 0xFF, 0xBF, 0xFF, 0x3F, 0x00, 0xFF, 0xBF,
        0x7F, 0xFF, 0x9F, 0xFF, 0xBF, 0xFF, 0xFF, 0x00, 0x00, 0xBF, 0x00, 0x00,
        0x70},
    .noise_divisors = {8, 16, 32, 48, 64, 80, 96, 112},
    .read = read,
    .write = write,
    .tick = tick,
    .step_pulse = step_pulse,
    .step_wave = step_wave,
    .step_noise = step_noise,
    .sequencer_step = sequencer_step,
    .reset = reset,
    .power_off = power_off,
    .trigger = trigger,
    .sweep_calculate = sweep_calculate,
    .clock_length = clock_length,
    .clock_sweep = clock_sweep,
    .clock_envelope = clock_envelope,
    .write_length_enable = write_length_enable,
    .channel_output = channel_output,
    .pcm = pcm,
    .emit_sample = emit_sample,
    .mix = mix,
    .audio_callback = audio_callback,
    .init_sound_system = init_sound_system,
    .update_volume = update_volume,
    .serialize = serialize,
};

const class_t *Sound = (const class_t *) &init_sound;
