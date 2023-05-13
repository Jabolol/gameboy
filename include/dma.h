#include "common.h"
#include "oop.h"

#ifndef __DMA
    #define __DMA

typedef struct gameboy_aux GameboyClass;
typedef struct dma_aux DMAClass;

typedef struct dma_aux {
    /* Properties */
    class_t metadata;
    GameboyClass *parent;
    dma_context_t *context;
    /* Methods */
    void (*start)(DMAClass *, uint8_t);
    void (*tick)(DMAClass *);
    bool (*transferring)(DMAClass *);
} DMAClass;

extern const class_t *DMA;
#endif
