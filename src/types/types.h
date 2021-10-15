#ifndef IRIS_TYPES_H
#define IRIS_TYPES_H

// todo: hide fields of structs from user, objects should be opaque 
// todo: consistent naming
// todo: implement vectors and make usage of them in dict implementation?

// todo: identifier symbols should be stored by RefCell as they're highly shared
//       also on collision we can check whether already existing string under the same name is the same
//       and handle collisions that way

#include <stddef.h>
#include <stdbool.h>

struct _IrisObject;
struct _IrisString;
struct _IrisList;
struct _IrisDict;

#include "types/list.h"
#include "types/string.h"
#include "types/dict.h"

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

typedef enum {
  okNone,
  okInt,
  okSymbol,
  okString,
  okList,
  okDict,
  okRefCell,
  N_OBJECT_KINDS
} ObjectKind;

// todo:
// typedef struct {
//   // if need for shared ownership arises - one can use reference counter
//   // it's implemented as object that holds some opaque identity value
//   // that corresponds with some object in global pool
//   // identities are untyped, but it's guaranteed that they point to valid IrisObject if validity check says so
//   size_t identity;
// } IrisRefCell;

void object_destroy(struct _IrisObject* obj);
void object_move(struct _IrisObject*);
// void object_move_dark_magic(ObjectKind kind, void* variant);
bool object_is_valid(struct _IrisObject);
size_t object_hash(struct _IrisObject);
void object_print(struct _IrisObject);

#endif
