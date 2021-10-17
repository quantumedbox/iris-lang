#ifndef IRIS_OBJECT_H
#define IRIS_OBJECT_H

/*
  Metamorphic object, used in dynamic runtime extensively
*/

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
struct _IrisFunc;
struct _IrisError;

#include "types/list.h"
#include "types/string.h"
#include "types/dict.h"
#include "types/func.h"
#include "types/error.h"

typedef enum {
  irisObjectKindNone,
  irisObjectKindError,
  irisObjectKindFunc,
  irisObjectKindInt,
  irisObjectKindFloat,
  irisObjectKindString,
  irisObjectKindList,
  irisObjectKindDict,
  N_OBJECT_KINDS
} IrisObjectKind;

typedef struct _IrisObject {
  // polymorphic container, mostly used for representing code as data
  // homogeneous containers should be proffered
  IrisObjectKind kind;
  union {
    int         int_variant; // todo: instead should use biggest available integer type that fits into register
    float       float_variant; // todo: use double on x64
    IrisString  string_variant;
    IrisList    list_variant;
    IrisDict    dict_variant;
    IrisFunc    func_variant;
    IrisError   error_variant;
  };
} IrisObject;

// todo:
// typedef struct {
//   // if need for shared ownership arises - one can use reference counter
//   // it's implemented as object that holds some opaque identity value
//   // that corresponds with some object in global pool
//   // identities are untyped, but it's guaranteed that they point to valid IrisObject if validity check says so
//   size_t identity;
// } IrisRefCell;

struct _IrisObject object_copy(const struct _IrisObject);
void object_destroy(struct _IrisObject*);
void object_move(struct _IrisObject*);
bool object_is_valid(const struct _IrisObject);
bool object_is_none(const struct _IrisObject);
size_t object_hash(const struct _IrisObject);

/*
  @brief  Print object as is, without connection to interpreter semantics
*/
void object_print(const struct _IrisObject, bool newline);

/*
  @brief  Print representation of object with type hints
          For example, string will have "" markers when printed
*/
void object_print_repr(const struct _IrisObject, bool newline);

#endif
