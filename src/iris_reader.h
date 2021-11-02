#ifndef IRIS_READER_H
#define IRIS_READER_H

#include "types/iris_types.h"

/*
  @brief  Apply name and macro resolution to given object
          Should be done before passing data to eval
  @params scope - new definitions will be wirtten it its local_scope
  @return Runnable list or error obj
*/
// IrisObject codelist_resolve(const IrisObject, IrisScopeUnformed* scope);
IrisObject codelist_resolve(const IrisObject, const IrisDict scope);

/*
  @brief  Apply default reader procedure to given string
  @return Codelist or error obj
*/
IrisObject string_read(const IrisString);

#endif
