#include <stdio.h>
#include "common.h"
#include "oop.h"

#ifndef __SNAPSHOT
    #define __SNAPSHOT

typedef struct gameboy_aux GameboyClass;
typedef struct snapshot_aux SnapshotClass;

typedef struct snapshot_aux {
    /* Properties */
    class_t metadata;
    GameboyClass *parent;
    const char magic[4];
    uint32_t version;
    uint8_t *data;
    size_t size;
    size_t capacity;
    size_t cursor;
    bool loading;
    bool failed;
    /* Methods */
    bool (*save)(SnapshotClass *, const char *);
    bool (*load)(SnapshotClass *, const char *);
    void (*field)(SnapshotClass *, void *, size_t);
    void (*serialize)(SnapshotClass *);
    void (*header)(SnapshotClass *);
    uint32_t (*layout)(SnapshotClass *);
} SnapshotClass;

extern const class_t *Snapshot;
#endif
