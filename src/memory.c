#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "memory.h"
#include "types/types.h"
#include "utils.h"

// todo: it's possible to log status and lifetime changes of every allocation
// todo: something similar to mcheck.h functionalities, we could trace double frees and validity of pointers as allocations

#ifdef IRIS_COLLECT_MEMORY_METRICS
// static IrisDict allocations;
static size_t n_allocations = 0ULL;
static size_t n_resizes = 0ULL;
static size_t n_frees = 0ULL;
// size_t memory_usage_current = 0ULL; // todo: for that we need to trace resizes which requires some additional work
// size_t memory_usage_peak = 0ULL;    //       we could probably use dictionary for that and use memory locations as keys
                                       //       tho there's problem with that as info about allocations will consume memory too
                                       //       so, info will be quite spoiled
#endif

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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
void* iris_standard_alloc(size_t bytes) {
  if (bytes == 0ULL) {
    return NULL;
  }
  void* mem = malloc(bytes);
  assert(pointer_is_valid(mem)); // todo: shouldn't be assert, but user code catch-able error
  #ifdef IRIS_COLLECT_MEMORY_METRICS
  n_allocations++;
  #endif
  return mem;
}
#pragma GCC diagnostic pop

void* iris_standard_resize(void* mem, size_t bytes) {
  if (mem == NULL) {
    return iris_standard_alloc(bytes);
  } else if (bytes == 0ULL) {
    iris_standard_free(mem);
    return NULL;
  }
  assert(pointer_is_valid(mem));
  void* resized = realloc(mem, bytes);
  assert(pointer_is_valid(resized)); // todo: shouldn't be assert, but user code catch-able error
  #ifdef IRIS_COLLECT_MEMORY_METRICS
  n_resizes++;
  #endif
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

void iris_metrics_print_repr() {
  #ifdef IRIS_COLLECT_MEMORY_METRICS
  (void)fputs("--- memory metrics:\n", stdout);
  (void)fprintf(stdout, "allocations: %llu\n", n_allocations);
  (void)fprintf(stdout, "deallocations: %llu, diff: %lld\n", n_frees, (long long int)n_allocations - (long long int)n_frees);
  (void)fprintf(stdout, "resizes: %llu\n", n_resizes);
  #else
  (void)fputs("--- memory metrics: no data was collected as collection was turned off on compilation, pass -DIRIS_COLLECT_MEMORY_METRICS to enable\n", stdout);
  #endif
}
