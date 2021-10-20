#ifndef IRIS_DICT_H
#define IRIS_DICT_H

#include <stddef.h>
#include <stdbool.h>

// todo: it's not convenient to have back-reference to pushed object
//       maybe dict_push_* should return hash key to which item was assigned?

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

IrisDict dict_new(void);
IrisDict dict_copy(const IrisDict);
void dict_push_object(IrisDict*, const struct _IrisObject key, struct _IrisObject* item);
void dict_push_string(IrisDict*, const struct _IrisObject key, struct _IrisString* item);
void dict_push_func(IrisDict*, const struct _IrisObject key, struct _IrisFunc* item);
void dict_push_list(IrisDict*, const struct _IrisObject key, struct _IrisList* item);
bool dict_has(const IrisDict, const struct _IrisObject key);
void dict_erase(IrisDict*, const struct _IrisObject key);
// void dict_erase_key(IrisDict*, size_t key); // todo: do we need it?

/*
  @brief  Get copy of object in dictionary
  @warn   Will panic if there's no item with given key, check dict_has() before calling
*/
struct _IrisObject dict_get(const IrisDict, const struct _IrisObject key);

// todo: should be strongly discouraged even in implementation of interpreter
/*
  @brief  Get reference to object in dictionary
  @warn   Returned reference is valid until the next mutation in dict
          Pointer should only be valid inside of function that called this, no passing it around
          You should only use this after the dictionary was formed and will not mutate
  @warn   Will panic if there's no item with given key, check dict_has() before calling
*/
const struct _IrisObject* dict_get_view(const IrisDict, const struct _IrisObject key);

bool dict_is_valid(const IrisDict);
void dict_destroy(IrisDict*);
void dict_move(IrisDict*);
void dict_print_repr(const IrisDict, bool newline);

#define dict_to_object(dict) (struct _IrisObject){ .kind = irisObjectKindDict, .dict_variant = dict }

#endif
