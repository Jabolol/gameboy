#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef __CLASS
    #define __CLASS
typedef void (*_constructor)(void *self, va_list *args);
typedef void (*_destructor)(void *self);
typedef struct {
    /* Size of the class */
    const size_t _size;
    /* Friendly name to debug */
    char *_name;
    /* Method to create the class */
    _constructor _constructor;
    /* Method to destroy the class */
    _destructor _destructor;
} class_t;
#endif

#ifndef __NEW
    #define __NEW
/* Create a new class */
void *new_class(const class_t *self, ...);
/* Helper function */
void *allocate_class(const class_t *self, va_list *ap);
/* Destroy the class */
void destroy_class(void *self);
#endif
