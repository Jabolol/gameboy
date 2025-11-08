#include "common.h"
#include "oop.h"

#ifndef __PIPELINE
    #define __PIPELINE

typedef struct gameboy_aux GameboyClass;
typedef struct pipeline_aux PipelineClass;

typedef struct pipeline_aux {
    /* Properties */
    class_t metadata;
    GameboyClass *parent;
    /* Methods */
    void (*fetch)(PipelineClass *);
    void (*process)(PipelineClass *);
    uint32_t (*fifo_pop)(PipelineClass *);
    void (*fifo_push)(PipelineClass *, uint32_t);
    bool (*fifo_add)(PipelineClass *);
    void (*fifo_reset)(PipelineClass *);
    void (*push_pixel)(PipelineClass *);
    void (*load_sprite_tile)(PipelineClass *);
    void (*load_sprite_data)(PipelineClass *, uint8_t);
    uint32_t (*fetch_sprite_pixels)(
        PipelineClass *, int32_t, uint32_t, uint8_t, bool);
    bool (*visible)(PipelineClass *);
    void (*load_window_tile)(PipelineClass *);
} PipelineClass;

extern const class_t *Pipeline;
#endif
