#ifndef IRIS_LIST_H
#define IRIS_LIST_H

#include <stddef.h>
#include <stdbool.h>

typedef struct _IrisList {
  // iris lists are arrays which do act like typical lisp linked lists
  // by making elements stored in reversed order which allows O(1) insertions to 'head'
  // tho we don't really have to conform to this tradition even?
  // optionally we could just provide special function interface for interpreting lists as linked lists
  // but that's really not a good idea, but we will see
  struct _IrisObject* items;
  size_t len;
  size_t cap;
} IrisList;

#include "types/types.h"

IrisList list_new();
void list_push_object(IrisList*, struct _IrisObject*);
void list_push_int(IrisList*, int);
void list_push_string(IrisList*, struct _IrisString*);
void list_push_list(IrisList*, IrisList*);
void list_destroy(IrisList*);
void list_move(IrisList*);
void list_print(IrisList, bool newline);
void list_print_debug(IrisList, bool newline);

#endif
