#ifndef MEMORY_H
#define MEMORY_H

#include <stdbool.h>

// warn: be cautious about allocators in different translation units, don't mix them
//       better to use one single solution in whole project
// todo: iris_resize0

typedef enum {
  irisPtrValid,           // given pointer is valid, but it doesn't mean it's valid everywhere
  irisPtrNull,            // given pointer is equal to NULL
  irisPtrInTextSection,   // given pointer intersects with text section of binary
} IrisPointerStatus;

bool is_pointer_valid(const void* p);
IrisPointerStatus pointer_status(const void* p);

void* iris_standard_alloc(size_t bytes);
void* iris_standard_resize(void* mem, size_t bytes);
void  iris_standard_free(void* mem);

void* iris_metrics_alloc(size_t bytes);
void* iris_metrics_resize(void* mem, size_t bytes);
void  iris_metrics_free(void* mem);

void iris_metrics_print();

// zero alloc that uses IRIS_ALLOC
void* iris_alloc0_untyped(size_t size);

#ifndef IRIS_ALLOC
  #ifdef IRIS_COLLECT_MEMORY_METRICS
    #define IRIS_ALLOC(size) iris_metrics_alloc(size)
    #define IRIS_RESIZE(ptr, size) iris_metrics_resize(ptr, size)
    #define IRIS_FREE(ptr) iris_metrics_free(ptr)
  #else
    #include <stdlib.h>
    #define IRIS_ALLOC(size) iris_standard_alloc(size)
    #define IRIS_RESIZE(ptr, size) iris_standard_resize(ptr, size)
    #define IRIS_FREE(ptr) iris_standard_free(ptr)
  #endif
#else
  #ifndef IRIS_RESIZE
    #error "no memory resize implementation"
  #endif
  #ifndef IRIS_FREE
    #error "no memory free implementation"
  #endif
#endif

// todo: do we need to check for NULL return from malloc? lol
//       or we should assume that everything is fucked at that point
//       or caller should assert for NULL by itself
#define iris_alloc(size, type) (type*)IRIS_ALLOC(size * sizeof(type))
#define iris_alloc0(size, type) (type*)iris_alloc0_untyped(size * sizeof(type))
#define iris_resize(ptr, size, type) (type*)IRIS_RESIZE(ptr, size * sizeof(type))
#define iris_free(ptr) IRIS_FREE(ptr)

#endif
