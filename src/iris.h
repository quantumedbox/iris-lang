#ifndef IRIS_H
#define IRIS_H

#define IRIS_BOUND_CHECK

// todo: rename to types.h? or something like that, iris.h should be external user include

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// todo: move it to memory.h
#ifndef IRIS_ALLOC
#define IRIS_ALLOC(size) malloc(size);
#define IRIS_RESIZE(ptr, size) realloc(ptr, size);
#define IRIS_FREE(ptr) free(ptr);
#endif

// todo: do we need to check for NULL return from malloc? lol
// or we should assume that everything is fucked at that point
// todo: make it inline function instead with possibility of gathering metrics
#define iris_alloc(size, type) (type*)IRIS_ALLOC(size * sizeof(type))
#define iris_resize(ptr, size, type) (type*)IRIS_RESIZE(ptr, size * sizeof(type))
#define iris_free(ptr) IRIS_FREE(ptr)

// todo: definition shouldn't be in header
__forceinline void* iris_alloc0_untyped(size_t size) {
  void* mem = IRIS_ALLOC(size);
  memset(mem, 0, size);
  return mem;
}
#define iris_alloc0(size, type) (type*)iris_alloc0_untyped(size * sizeof(type));

enum ObjectKind {
  okNone,
  okInt,
  okSymbol,
  okString,
  okList,
  N_OBJECT_KINDS
};

typedef struct {
  // iris byte strings are immutable and not null terminated
  // they don't have eny encoding attached by themselves
  size_t len;
  char* data;
} IrisString;

struct _IrisObject;
typedef struct _IrisList {
  // iris lists are arrays which do act like typical lisp linked lists
  // by making elements stored in reversed order which allows O(1) insertions to 'head'
  struct _IrisObject* items;
  size_t len;
  size_t cap;
} IrisList;

typedef struct _IrisObject {
  // polymorphic container, mostly used for representing code as data
  int kind;
  union {
    int           int_variant;
    IrisString    symbol_variant;
    IrisList      list_variant;
  };
} IrisObject; // variant object

IrisList new_list();
void push_object(IrisList*, IrisObject);
void push_int(IrisList*, int);
void push_symbol(IrisList*, IrisString);
void push_list(IrisList*, IrisList);
void free_list(IrisList);

IrisString string_from_chars(const char*);
IrisString string_from_file(FILE*);
char nth_char(IrisString, size_t idx);
// IrisString string_from_slice(IrisString, size_t low, size_t high);
void free_string(IrisString);

#endif
