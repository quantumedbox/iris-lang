#include <string.h>
#include <stdio.h>
#include <assert.h>

// #include "types/dict.h"
#include "types/types.h"
#include "utils.h"
#include "memory.h"

#define DICT_PREALLOC 4U
#define DICT_GROW_FACTOR 4U // == 1 - (1 / 4) // capacity multiplies by two when factor is equal or greater than cardinality

static_assert(DICT_PREALLOC > 0U && (DICT_PREALLOC & (DICT_PREALLOC - 1U)) == 0U, "starting bucket capacity of dict should be power of 2");
static_assert(DICT_GROW_FACTOR > 1U, "invalid dict grow factor");

typedef struct {
  size_t key; // hash
  IrisObject item;
} IrisDictPair;

typedef struct _IrisDictBucket {
  // dict buckets are implemented as vectors
  IrisDictPair* pairs;
  size_t len;
} IrisDictBucket;

IrisDict dict_new() {
  IrisDict result = {
    .buckets = iris_alloc0(DICT_PREALLOC, IrisDictBucket),
    .cap = DICT_PREALLOC,
    .card = 0ULL,
  };
  return result;
}

// todo: we shouldn't have two versions of funcs, they make maintaining harder
__forceinline void dict_free_buckets(IrisDictBucket* buckets, size_t len) {
  assert(pointer_is_valid(buckets));
  for (size_t b = 0ULL; b < len; b++) {
    assert((buckets[b].len > 0 && pointer_is_valid(buckets[b].pairs)) ||
      (buckets[b].len == 0 && !pointer_is_valid(buckets[b].pairs))
    );
    for (size_t p = 0ULL; p < buckets[b].len; p++) {
      object_destroy(&buckets[b].pairs[p].item);
    }
    if (buckets[b].len > 0 && pointer_is_valid(buckets[b].pairs)) {
      iris_free(buckets[b].pairs);
    }
  }
  iris_free(buckets);
}

/*
  @brief  dict_free_buckets version that assumes that pairs were moved and shouldn't be freed
*/
__forceinline void dict_free_buckets_after_move(IrisDictBucket* buckets, size_t len) {
  assert(pointer_is_valid(buckets));
  for (size_t b = 0ULL; b < len; b++) {
    assert((buckets[b].len > 0 && pointer_is_valid(buckets[b].pairs)) ||
      (buckets[b].len == 0 && !pointer_is_valid(buckets[b].pairs))
    );
    if (buckets[b].len > 0 && pointer_is_valid(buckets[b].pairs)) {
      iris_free(buckets[b].pairs);
    }
  }
  iris_free(buckets);
}

__forceinline void
dict_move_pair(IrisDictBucket* buckets,
               size_t len,
               IrisDictPair pair)
{
  size_t idx = pair.key % len;
  buckets[idx].pairs = iris_resize(buckets[idx].pairs, buckets[idx].len + 1ULL, IrisDictPair);
  buckets[idx].pairs[buckets[idx].len] = pair;
  buckets[idx].len++;
}

__forceinline void dict_grow(IrisDict* dict) {
  if (dict->card >= (dict->cap - (dict->cap / DICT_GROW_FACTOR))) {
    size_t new_cap = dict->cap << 1ULL;
    IrisDictBucket* new_buckets = iris_alloc0(new_cap, IrisDictBucket);
    for (size_t b = 0; b < dict->cap; b++) {
      for (size_t p = 0; p < dict->buckets[b].len; p++) {
        dict_move_pair(new_buckets, new_cap, dict->buckets[b].pairs[p]);
      }
    }
    dict_free_buckets_after_move(dict->buckets, dict->cap);
    dict->buckets = new_buckets;
    dict->cap = new_cap;
  }
}

// todo: that's quite badly written, make it better structured
/*
  @brief  Internal push function that is agnostic to item kind
          At this point it owns the reference to originally passed object
          and is required to eliminate dangling pointers and invalid sizes
          Be very cautious with this fuckery!
*/
#define dict_push(m_dict, m_hash_key, m_item, m_type, m_variant, m_kind) {                              \
  dict_grow(m_dict);                                                                                    \
  size_t dict_push_pair_idx = (m_hash_key) % m_dict->cap;                                               \
  for (size_t i = 0; i < m_dict->buckets[dict_push_pair_idx].len; i++) {                                \
    if (m_dict->buckets[dict_push_pair_idx].pairs[i].key == (m_hash_key)) {                             \
      m_variant##_destroy(&m_dict->buckets[dict_push_pair_idx].pairs[i].item.m_variant##_variant);      \
      m_dict->buckets[dict_push_pair_idx].pairs[i].item.kind = m_kind;                                  \
      m_dict->buckets[dict_push_pair_idx].pairs[i].item.m_variant##_variant = *(m_type*)m_item;         \
      return;                                                                                           \
    }                                                                                                   \
  }                                                                                                     \
  m_dict->buckets[dict_push_pair_idx].pairs = iris_resize(                                              \
    m_dict->buckets[dict_push_pair_idx].pairs,                                                          \
    m_dict->buckets[dict_push_pair_idx].len + 1ULL,                                                     \
    IrisDictPair                                                                                        \
  );                                                                                                    \
  IrisObject dict_push_obj = { .kind = m_kind, .m_variant##_variant = *(m_type*)m_item };               \
  IrisDictPair dict_push_pair = { .key = (m_hash_key), .item = dict_push_obj };                         \
  m_dict->buckets[dict_push_pair_idx].pairs[m_dict->buckets[dict_push_pair_idx].len] = dict_push_pair;  \
  m_dict->buckets[dict_push_pair_idx].len++;                                                            \
  m_dict->card++;                                                                                       \
}

void dict_push_object(IrisDict* dict, size_t key, IrisObject* obj) {
  switch (obj->kind) {
    case irisObjectKindString:
      dict_push(dict, key, (void*)(&obj->string_variant), IrisString, string, irisObjectKindString);
      string_move(&obj->string_variant);
      break;
    case irisObjectKindFunc:
      dict_push(dict, key, (void*)(&obj->func_variant), IrisFunc, func, irisObjectKindFunc);
      func_move(&obj->func_variant);
      break;
    default:
      panic("push to dict behavior isn't defined for object variant");
  }
}

void dict_push_string(IrisDict* dict, IrisString* str) {
  assert(string_is_valid(*str));
  dict_push(dict, str->hash, (void*)str, IrisString, string, irisObjectKindString);
  string_move(str);
}

void dict_push_func(IrisDict* dict, struct _IrisString* symbol, struct _IrisFunc* func) {
  assert(func_is_valid(*func));
  assert(string_is_valid(*symbol));
  dict_push(dict, symbol->hash, (void*)func, IrisFunc, func, irisObjectKindFunc);
  func_move(func);
  string_destroy(symbol);
}

// todo: should it resize the memory block?
//       it should be okay that bucket forgets its capacity, as it will not attempt to use junk
//       but memory will be freed only on next insertion in the same bucket which is quite random
//       we could probably introduce some switch IRIS_KEEP_MINIMUM_RAM_USAGE for always shrinking memory to minimum
void dict_erase_by_key(IrisDict* dict, size_t key) {
  iris_check(dict_has(*dict, key), "attempt to erase nonexistent key in dict"); // todo: could be inlined into the end of for
  size_t idx = key % dict->cap;
  for (size_t i = 0; i < dict->buckets[idx].len; i++) {
    if (dict->buckets[idx].pairs[i].key == key) {
      object_destroy(&dict->buckets[idx].pairs[i].item);
      dict->buckets[idx].pairs[i].item = dict->buckets[idx].pairs[dict->buckets[idx].len - 1ULL].item;
      dict->buckets[idx].len--;
    }
  }
}

bool dict_has(const IrisDict dict, size_t key) {
  size_t idx = key % dict.cap;
  for (size_t i = 0; i < dict.buckets[idx].len; i++) {
    if (dict.buckets[idx].pairs[i].key == key) {
      return true;
    }
  }
  return false;
}

const IrisObject* dict_get_view(const IrisDict* dict, size_t key) {
  iris_check(dict_has(*dict, key), "attempt to get view of nonexistent key in dict"); // todo: could be inlined into the end of for
  size_t idx = key % dict->cap;
  for (size_t i = 0; i < dict->buckets[idx].len; i++) {
    if (dict->buckets[idx].pairs[i].key == key) {
      return &dict->buckets[idx].pairs[i].item;
    }
  }
  __builtin_unreachable();
}

// todo: maybe it should check how well each bucket is formed too 
bool dict_is_valid(const IrisDict dict) {
  return pointer_is_valid(dict.buckets);
}

void dict_destroy(IrisDict* dict) {
  assert(pointer_is_valid(dict));
  dict_free_buckets(dict->buckets, dict->cap);
  dict_move(dict);
}

void dict_move(IrisDict* dict) {
  dict->buckets = NULL;
  dict->card = 0ULL;
  dict->cap = 0ULL;
}

void dict_print_repr(const IrisDict dict, bool newline) {
  (void)fputc('{', stdout);
  bool put_comma = false;
  for (size_t b = 0; b < dict.cap; b++) {
    for (size_t p = 0; p < dict.buckets[b].len; p++) {
      if (!put_comma) {
        put_comma = true;
      } else {
        (void)fputs(", ", stdout);
      }
      (void)fprintf(stdout, "%llu: ", dict.buckets[b].pairs[p].key);
      object_print_repr(dict.buckets[b].pairs[p].item, false);
    }
  }
  (void)fputc('}', stdout);
  if (newline) { (void)fputc('\n', stdout); }
  fflush(stdout);
}
