#ifndef IRIS_MEMORY_H
#define IRIS_MEMORY_H

#include <stdbool.h>

typedef enum {
  irisPtrValid,           // given pointer is valid, but it doesn't mean it's valid everywhere
  irisPtrNull,            // given pointer is equal to NULL
  irisPtrInTextSection,   // given pointer intersects with text section of binary
} IrisPointerStatus;

bool pointer_is_valid(const void* p);
IrisPointerStatus pointer_status(const void* p);

void* iris_standard_alloc(size_t bytes);
void* iris_standard_resize(void* mem, size_t bytes);
void  iris_standard_free(void* mem);
// zero alloc that uses IRIS_ALLOC
void* iris_alloc0_untyped(size_t size);
void iris_metrics_print_repr(void);

#ifndef IRIS_ALLOC
  #define IRIS_ALLOC(size) iris_standard_alloc(size)
  #define IRIS_RESIZE(ptr, size) iris_standard_resize(ptr, size)
  #define IRIS_FREE(ptr) iris_standard_free(ptr)
#else
  #ifndef IRIS_RESIZE
    #error "no memory resize implementation"
  #endif
  #ifndef IRIS_FREE
    #error "no memory free implementation"
  #endif
#endif

// todo: macroses are evil, maybe should make something else
#define iris_alloc(size, type) (type*)IRIS_ALLOC((size) * sizeof(type))
#define iris_alloc0(size, type) (type*)iris_alloc0_untyped((size) * sizeof(type))
#define iris_resize(ptr, size, type) (type*)IRIS_RESIZE(ptr, (size) * sizeof(type))
#define iris_free(ptr) IRIS_FREE(ptr)

#endif
