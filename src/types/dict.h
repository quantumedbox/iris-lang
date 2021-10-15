#ifndef IRIS_DICT_H
#define IRIS_DICT_H

#include <stddef.h>
#include <stdbool.h>

typedef struct _IrisDict {
  // iris hash tables only store hashes of objects and are designed mostly for lookup of module scopes
  // as original objects from which hash is coming aren't saved you can't use them again
  // but you really shouldn't in the first place, hash tables aren't designed for that
  // order isn't preserved
  // frankly, current implementation doesn't really care about distributions and probings, we might focus on that in the future
  struct _IrisDictBucket* buckets;
  size_t card; // cardinality aka amount of key/item pairs
  size_t cap;  // allocated buckets
} IrisDict;

#include "types/types.h"

IrisDict dict_new();
void dict_push_object(IrisDict*, size_t key, struct _IrisObject* item);
void dict_push_string(IrisDict*, struct _IrisString* str);
bool dict_has(IrisDict, size_t key);
void dict_destroy(IrisDict*);
void dict_move(IrisDict*);
void dict_print(IrisDict, bool newline);

#endif
