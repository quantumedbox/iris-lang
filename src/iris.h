#ifndef IRIS_H
#define IRIS_H

#include <stdio.h>

enum ObjectKind {
  okInt,
  okSymbol,
  okString,
  okList,
  N_OBJECT_KINDS
};

typedef struct {
  // iris strings are immutable and not null terminated
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
void push_int(IrisList*, int);
void push_symbol(IrisList*, IrisString);
void push_list(IrisList*, IrisList);
void free_list(IrisList);

IrisString string_from_chars(const char*);
IrisString string_from_file(FILE*);
void free_string(IrisString);

#endif
