#include <stdlib.h>

#include "types/types.h"
#include "utils.h"

// #include "types/string.h"
// #include "types/list.h"
// #include "types/dict.h"

// todo: push_* functions should have specified behaviour
//       but should they move values or copy them?
//       we probably need another class of functions for copies specifically
//       also we need to invalidate original moved objects, so, maybe require passing mutable pointer?
// todo: implement move_* functions for types

void object_move(IrisObject* obj) {
  switch (obj->kind) {
    case okInt: break;
    case okString:
      string_move(&obj->string_variant);
      break;
    case okList:
      list_move(&obj->list_variant);
      break;
    case okDict:
      dict_move(&obj->dict_variant);
    default:
      panic("move behavior for object variant isn't defined");
  }
}

/*
  @brief  Could be problematic and in general isn't preferable way of doing shit
          But in some places it's more convenient to do shit this way
          Probably will be changed in the future
*/
// void object_move_dark_magic(ObjectKind kind, void* variant) {
//   switch (kind) {
//     case okInt: break;
//     case okString:
//       string_move((IrisString*)variant);
//       break;
//     case okList:
//       list_move((IrisList*)variant);
//       break;
//     case okDict:
//       dict_move((IrisDict*)variant);
//     default:
//       panic("dark magic move behavior for object variant isn't defined");
//   }
// }

size_t object_hash(IrisObject obj) {
  switch (obj.kind) {
    case okInt:
      return obj.int_variant;
    case okString:
      return obj.string_variant.hash;
    default:
      panic("hash behavior for object variant isn't defined");
  }
}

bool object_is_valid(IrisObject obj) {
  switch (obj.kind) {
    case okInt:
      return true;
    case okString:
      return string_is_valid(obj.string_variant);
    default:
      panic("validity check for object variant isn't defined");
  }
}

void object_destroy(IrisObject* obj) {
  switch (obj->kind) {
    case okNone:
      panic("attempt to destroy None object");
      break;
    case okInt: break;
    case okString:
      string_destroy(&obj->string_variant);
      break;
    case okList:
      list_destroy(&obj->list_variant);
      break;
    default:
      panic("destroy behavior for object variant isn't defined");
  }
}

void object_print(IrisObject obj) {
  switch (obj.kind) {
    case okNone:
      (void)fprintf(stdout, "None!");
      break;
    case okList:
      list_print(obj.list_variant, false);
      break;
    case okInt:
      (void)fprintf(stdout, "%d", obj.int_variant);
      break;
    case okString:
      string_print(obj.string_variant, false);
      break;
    default: panic("printing behaviour for obj type isn't defined");
  }
}
