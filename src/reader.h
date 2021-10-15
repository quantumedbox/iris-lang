#ifndef IRIS_READER_H
#define IRIS_READER_H

#include "types/string.h"
#include "types/list.h"

typedef IrisList (*IrisReader)(IrisString);

IrisList nurture(IrisString);

#endif
