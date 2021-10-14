#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "memory.h"
#include "utils.h"

// todo: it's possible to log status and lifetime changes of every allocation
// todo: something similar to mcheck.h functionalities by IRIS_CHECK_ALLOCATIONS

bool is_pointer_valid(const void* p) {
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

unsigned int n_allocations = 0U;
unsigned int n_resizes = 0U;
unsigned int n_frees = 0U;

void* iris_standard_alloc(size_t bytes) {
  warning(bytes == 0, "0 sized allocations aren't defined");
  void* mem = malloc(bytes);
  assert(is_pointer_valid(mem));
  return mem;
}

void* iris_standard_resize(void* mem, size_t bytes) {
  if (mem == NULL) {
    return iris_metrics_alloc(bytes);
  }
  void* resized = realloc(mem, bytes);
  assert(is_pointer_valid(mem));
  return resized;
}

void iris_standard_free(void* mem) {
  assert(is_pointer_valid(mem));
  free(mem);
}

void* iris_alloc0_untyped(size_t bytes) {
  void* mem = IRIS_ALLOC(bytes);
  memset(mem, 0, bytes);
  return mem;
}

void* iris_metrics_alloc(size_t bytes) {
  warning(bytes == 0, "0 sized allocations aren't defined");
  void* mem = malloc(bytes);
  assert(is_pointer_valid(mem));
  n_allocations++;
  return mem;
}

void* iris_metrics_resize(void* mem, size_t bytes) {
  if (mem == NULL) {
    return iris_metrics_alloc(bytes);
  }
  void* resized = realloc(mem, bytes);
  assert(is_pointer_valid(mem));
  n_resizes++;
  return resized;
}

void iris_metrics_free(void* mem) {
  assert(is_pointer_valid(mem));
  free(mem);
  n_frees++;
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
