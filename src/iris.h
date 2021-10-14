#ifndef IRIS_H
#define IRIS_H

// todo: move it to build solution
#define IRIS_BOUND_CHECK

// todo: rename to types.h? or something like that, iris.h should be external include
// todo: strings should be immutable, but there should be a way for constructing them in parts, something like 'string_builder' type
// todo: strings should be hashed on creation
// todo: hide fields of structs from user, objects should be opaque 
// todo: consistent naming
// todo: currently pushing and freeing doesn't change pushed and freed objects as they're not passed by ref
//       it might produce dangling pointers that are impossible to diagnose
// todo: implement vectors and make usage of them in dict implementation?

// warn! anything prefixed with 'push' invalidates passed objects

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "memory.h"

enum ObjectKind {
  okNone,
  okInt,
  okSymbol,
  okString,
  okList,
  okDict,
  okRefCell,
  N_OBJECT_KINDS
};

typedef struct {
  // iris byte strings are immutable and not null terminated
  // they don't have eny encoding attached and are hashed on creation
  // todo: should we allow strings of 0 length?
  char* data;
  size_t len;
  size_t hash;
} IrisString;

typedef struct {
  // iris lists are arrays which do act like typical lisp linked lists
  // by making elements stored in reversed order which allows O(1) insertions to 'head'
  struct _IrisObject* items;
  size_t len;
  size_t cap;
} IrisList;

typedef struct {
  // iris hash tables only store hashes of objects and are designed mostly for lookup of module scopes
  // as original objects from which hash is coming aren't saved you can't use them again
  // but you really shouldn't in the first place, hash tables aren't designed for that
  // order isn't preserved
  // frankly, current implementation doesn't really care about distributions and probings, we might focus on that in the future
  struct _IrisDictBucket* buckets;
  size_t card; // cardinality aka amount of key/item pairs
  size_t cap;  // allocated buckets
} IrisDict;

// todo:
typedef struct {
  // if need for shared ownership arises - one can use reference counter
  // it's implemented as object that holds some opaque identity value
  // that corresponds with some object in global pool
  // identities are untyped, but it's guaranteed that they point to valid IrisObject if validity check says so
  size_t identity;
} IrisRefCell;

typedef struct _IrisObject {
  // polymorphic container, mostly used for representing code as data
  // homogeneous containers should be proffered
  int kind;
  union {
    int         int_variant;
    IrisString  string_variant;
    IrisList    list_variant;
    IrisDict    dict_variant;
  };
} IrisObject;

#define string_to_object(str) (IrisObject){ .kind = okString, .string_variant = str }

void free_object(IrisObject*);

IrisList new_list();
void push_object(IrisList*, IrisObject*);
void push_int(IrisList*, int);
void push_string(IrisList*, IrisString*);
void push_list(IrisList*, IrisList*);
void free_list(IrisList*);

IrisDict dict_new();
void dict_push_object(IrisDict*, size_t key, IrisObject* item);
bool dict_has(IrisDict, size_t key);
void dict_free(IrisDict*);

bool string_is_valid(IrisString str);
IrisString string_from_chars(const char*);
IrisString string_from_file(FILE*);
IrisString string_from_view(const char* low, const char* high);
char nth_char(IrisString, size_t idx);
void free_string(IrisString*);

void print_string(IrisString, bool newline);
void print_string_debug(IrisString, bool newline);
void print_list(IrisList, bool newline);
void print_list_debug(IrisList, bool newline);
void print_dict(IrisDict, bool newline);

#endif
