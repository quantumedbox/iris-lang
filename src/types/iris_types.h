#ifndef IRIS_TYPES_H
#define IRIS_TYPES_H

// todo: for now pretty much all types are dependent on IrisObject
//       so we have to import them relative to it
//       maybe there's better way to resolve this, for example don't implement IrisObject related functionalities
//       when IRIS_OBJECT_H isn't defined or something
#include "types/iris_object.h"

typedef struct {
  // Yet unformed scope used in reader to fill in local definitions
  const IrisList inhereted_scopes;  // immutable formed scopes
  IrisDict       local_scope;       // interpreter-local scope that could be modified
} IrisScopeUnformed;

typedef struct {
  const IrisList inhereted_scopes;  // immutable formed scopes
  const IrisDict local_scope;       // interpreter-local scope that could be modified
} IrisScope;

inline IrisScope iris_scope_form(const IrisScopeUnformed scope) {
  return (IrisScope){
    .inhereted_scopes = scope.inhereted_scopes,
    .local_scope = scope.local_scope
  };
}

#endif
