#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "memory.h"
#include "utils.h"

// todo: it's possible to log status and lifetime changes of every allocation
// todo: something similar to mcheck.h functionalities, we could trace double frees and validity of pointers as allocations

bool pointer_is_valid(const void* p) {
  // extern char etext;
  return (p != NULL); // && ((char*) p > &etext);
}

IrisPointerStatus pointer_status(const void* p) {
  // extern char etext;
  if (p == NULL) {
    return irisPtrNull;
  // } else if ((char*) p <= &etext) {
  //   return irisPtrInTextSection;
  } else {
    return irisPtrValid;
  }
}

#ifdef IRIS_COLLECT_MEMORY_METRICS
unsigned int n_allocations = 0U;
unsigned int n_resizes = 0U;
unsigned int n_frees = 0U;
#endif

void* iris_standard_alloc(size_t bytes) {
  if (bytes == 0) {
    return NULL;
  }
  void* mem = malloc(bytes);
  assert(pointer_is_valid(mem));
  #ifdef IRIS_COLLECT_MEMORY_METRICS
  n_allocations++;
  #endif
  // printf("%x\n", mem);
  return mem;
}

void* iris_standard_resize(void* mem, size_t bytes) {
  if (mem == NULL) {
    return iris_standard_alloc(bytes);
  } else if (bytes == 0) {
    iris_standard_free(mem);
    return NULL;
  }
  assert(pointer_is_valid(mem));
  void* resized = realloc(mem, bytes);
  assert(pointer_is_valid(resized)); // todo: shouldn't be assert, but user code catch-able error
  #ifdef IRIS_COLLECT_MEMORY_METRICS
  n_resizes++;
  #endif
  // printf("%x\n", resized);
  return resized;
}

void iris_standard_free(void* mem) {
  assert(pointer_is_valid(mem));
  free(mem);
  #ifdef IRIS_COLLECT_MEMORY_METRICS
  n_frees++;
  #endif
}

void* iris_alloc0_untyped(size_t bytes) {
  void* mem = IRIS_ALLOC(bytes);
  memset(mem, 0, bytes);
  return mem;
}

void iris_metrics_print() {
  #ifdef IRIS_COLLECT_MEMORY_METRICS
  (void)fprintf(stdout, "%s\n", "-- memory metrics:");
  (void)fprintf(stdout, "allocations: %u\n", n_allocations);
  (void)fprintf(stdout, "deallocations: %u, leaked: %d\n", n_frees, (int)n_allocations - (int)n_frees);
  (void)fprintf(stdout, "resizes: %u\n", n_resizes);
  #else
  (void)fprintf(stdout, "-- memory metrics: no data was collected as collection was turned off on compilation");
  #endif
}
