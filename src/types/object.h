#ifndef IRIS_OBJECT_H
#define IRIS_OBJECT_H

// todo: hide fields of structs from user, objects should be opaque
// todo: compile-time option for size of integer type, for example, IRIS_INT_PORTABLE that forces single size of integers and all architectures

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

struct _IrisObject;
struct _IrisString;
struct _IrisList;
struct _IrisDict;
struct _IrisFunc;
struct _IrisError;
struct _IrisRefCell;

#include "types/list.h"
#include "types/string.h"
#include "types/dict.h"
#include "types/func.h"
#include "types/error.h"
#include "types/refcell.h"

typedef enum {
  irisObjectKindNone,
  irisObjectKindError,
  irisObjectKindRefCell,
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
    intmax_t    int_variant;
    float       float_variant;  // todo: use double on x64
    IrisString  string_variant;
    IrisList    list_variant;
    IrisDict    dict_variant;
    IrisFunc    func_variant;
    IrisRefCell refcell_variant;
    IrisError   error_variant;
  };
} IrisObject;

IrisObject object_copy(const IrisObject);
void object_destroy(IrisObject*);
void object_move(IrisObject*);
bool object_is_valid(const IrisObject);
bool object_is_none(const IrisObject);
size_t object_hash(const IrisObject);
bool object_equal(const IrisObject, const IrisObject);

/*
  @brief  Print object as is, without connection to interpreter semantics
*/
void object_print(const IrisObject, bool newline);

/*
  @brief  Print representation of object with type hints
          For example, string will have "" markers when printed
*/
void object_print_repr(const IrisObject, bool newline);

#define int_to_object(i) (IrisObject){ .kind = irisObjectKindInt, .int_variant = i }
#define float_to_object(f) (IrisObject){ .kind = irisObjectKindFloat, .float_variant = f }

#endif
