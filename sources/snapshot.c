#include "../include/gameboy.h"

static void constructor(void *ptr, va_list *args)
{
    SnapshotClass *self = (SnapshotClass *) ptr;
    self->parent = va_arg(*args, GameboyClass *);
}

static void destructor(void *ptr)
{
    SnapshotClass *self = (SnapshotClass *) ptr;
    free(self->data);
}

static void field(SnapshotClass *self, void *data, size_t size)
{
    if (self->loading) {
        self->failed |= self->cursor + size > self->size;
        if (!self->failed) {
            memcpy(data, self->data + self->cursor, size);
        }
    } else {
        if (self->cursor + size > self->capacity) {
            size_t capacity = (self->cursor + size) * 2;
            uint8_t *grown = realloc(self->data, capacity);
            if (!grown) {
                HANDLE_ERROR("failed memory allocation");
            }
            self->data = grown;
            self->capacity = capacity;
        }
        memcpy(self->data + self->cursor, data, size);
    }
    self->cursor += size;
}

static uint32_t layout(SnapshotClass UNUSED *self)
{
    return sizeof(cpu_context_t) + sizeof(ram_context_t)
        + sizeof(timer_context_t) + sizeof(lcd_context_t)
        + sizeof(ppu_context_t) + sizeof(cartridge_context_t)
        + sizeof(sound_channel_t);
}

static void header(SnapshotClass *self)
{
    rom_header_t *rom = self->parent->cartridge->context->header;
    char magic[4];
    uint32_t expected[4] = {self->version, self->layout(self),
        (rom->global_checksum << 8) | rom->checksum, self->size};
    uint32_t actual[4];

    memcpy(magic, self->magic, sizeof(magic));
    memcpy(actual, expected, sizeof(actual));
    self->field(self, magic, sizeof(magic));
    self->field(self, actual, sizeof(actual));
    self->failed |= memcmp(magic, self->magic, sizeof(magic))
        || memcmp(actual, expected, sizeof(actual));
}

static void serialize(SnapshotClass *self)
{
    GameboyClass *gameboy = self->parent;

    gameboy->serialize(gameboy, self);
    gameboy->cpu->serialize(gameboy->cpu, self);
    gameboy->ram->serialize(gameboy->ram, self);
    gameboy->timer->serialize(gameboy->timer, self);
    gameboy->io->serialize(gameboy->io, self);
    gameboy->joypad->serialize(gameboy->joypad, self);
    gameboy->dma->serialize(gameboy->dma, self);
    gameboy->lcd->serialize(gameboy->lcd, self);
    gameboy->ppu->serialize(gameboy->ppu, self);
    gameboy->sound->serialize(gameboy->sound, self);
    gameboy->cartridge->serialize(gameboy->cartridge, self);
}

static bool save(SnapshotClass *self, const char *path)
{
    FILE *stream;

    self->loading = false;
    self->failed = false;
    self->cursor = 0;
    self->size = 0;
    self->header(self);
    self->serialize(self);
    self->size = self->cursor;
    uint32_t size = self->size;
    memcpy(self->data + sizeof(self->magic) + 3 * sizeof(uint32_t), &size,
        sizeof(size));
    if (!((stream = fopen(path, "wb")))) {
        return false;
    }
    self->failed |=
        fwrite(self->data, 1, self->cursor, stream) != self->cursor;
    fclose(stream);
    return !self->failed;
}

static bool load(SnapshotClass *self, const char *path)
{
    FILE *stream = fopen(path, "rb");

    if (!stream) {
        return false;
    }
    fseek(stream, 0, SEEK_END);
    self->size = ftell(stream);
    rewind(stream);
    free(self->data);
    self->data = malloc(self->size);
    self->capacity = self->size;
    self->failed =
        !self->data || fread(self->data, 1, self->size, stream) != self->size;
    fclose(stream);

    self->loading = true;
    self->cursor = 0;
    self->header(self);
    if (self->failed) {
        return false;
    }
    self->serialize(self);
    return !self->failed;
}

const SnapshotClass init_snapshot = {
    {
        ._size = sizeof(SnapshotClass),
        ._name = "Snapshot",
        ._constructor = constructor,
        ._destructor = destructor,
    },
    .magic = {'G', 'B', 'S', 'T'},
    .version = 1,
    .save = save,
    .load = load,
    .field = field,
    .serialize = serialize,
    .header = header,
    .layout = layout,
};

const class_t *Snapshot = (const class_t *) &init_snapshot;
