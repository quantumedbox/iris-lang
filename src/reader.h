#ifndef IRIS_READER_H
#define IRIS_READER_H

#include "types/types.h"

// typedef IrisList (*IrisReader)(IrisString);

// IrisList nurture(IrisString);

/*
  @brief  Apply name and macro resolution to given object
          Should be done before passing data to eval
  @return Runnable list or error
*/
IrisObject codelist_resolve(const IrisObject, const IrisDict scope);

/*
  @brief  Apply default reader procedure to given string
  @return Codelist or error
*/
IrisObject string_read(const IrisString);

#endif
