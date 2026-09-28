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
    void (*line_start)(PipelineClass *);
    void (*load_sprite_data)(PipelineClass *);
    void (*rasterize_sprites)(PipelineClass *);
    void (*render)(PipelineClass *, uint8_t);
    void (*render_background)(PipelineClass *, uint8_t, uint8_t);
    void (*fetch_tile)(PipelineClass *, uint16_t, uint8_t, uint8_t[3]);
    uint32_t (*mix_pixel)(PipelineClass *, uint8_t);
    bool (*window_visible)(PipelineClass *);
} PipelineClass;

extern const class_t *Pipeline;
#endif
