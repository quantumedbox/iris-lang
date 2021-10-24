#ifndef IRIS_INTER_H
#define IRIS_INTER_H

#include <stdbool.h>

#include "types/iris_types.h"

// todo: interpreter should probably start with codestring, not codelist
//       to then resolve it by scopes of its own
// todo: ability to specify inhereted scopes of interpreters

// opaque type for interfacing interpreters
typedef struct _IrisInterThread* IrisInterHandle;

IrisInterHandle inter_new(void);
void inter_destroy(IrisInterHandle*);

/*
  @brief  Start new interpreter instance
  @return False on error, otherwise true
*/
bool inter_eval_codelist(IrisInterHandle*, IrisList*);

// bool inter_eval_codestring(IrisInterHandle*, IrisString*);
// bool inter_eval_file(IrisInterHandle*, const IrisString filename);
IrisObject inter_result(IrisInterHandle*);

#endif
