#include "../include/gameboy.h"

static void init(CPUClass __attribute__((unused)) * self)
{
    return;
}

static bool step(CPUClass __attribute__((unused)) * self)
{
    NOT_IMPLEMENTED();
    return false;
}

const CPUClass init_CPU = {
    {
        ._size = sizeof(CPUClass),
        ._name = "CPU",
        ._constructor = NULL,
        ._destructor = NULL,
    },
    .init = init,
    .step = step,
};

const class_t *CPU = (const class_t *) &init_CPU;
