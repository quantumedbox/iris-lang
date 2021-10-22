#include <string.h>
#include <stdio.h>
#include <assert.h>

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

IrisDict dict_copy(const IrisDict dict) {
  assert(dict_is_valid(dict));
  IrisDict result = {
    .buckets = iris_alloc(dict.cap, IrisDictBucket),
    .cap = dict.cap,
    .card = dict.card,
  };
  for (size_t b = 0ULL; b < dict.cap; b++) {
    result.buckets[b].pairs = iris_alloc(dict.buckets[b].len, IrisDictPair);
    result.buckets[b].len = dict.buckets[b].len;
    for (size_t p = 0ULL; p < dict.buckets[b].len; p++) {
      result.buckets[b].pairs[p].item = object_copy(dict.buckets[b].pairs[p].item);
      result.buckets[b].pairs[p].key = dict.buckets[b].pairs[p].key;
    }
  }
  return result;
}

// todo: we shouldn't have two versions of funcs, they make maintaining harder
// todo: inline in dict_destroy?
__forceinline void dict_free_buckets(IrisDictBucket* buckets, size_t len) {
  assert(pointer_is_valid(buckets));
  for (size_t b = 0ULL; b < len; b++) {
    assert((buckets[b].len > 0 && pointer_is_valid(buckets[b].pairs)) ||
      (buckets[b].len == 0 && !pointer_is_valid(buckets[b].pairs))
    );
    for (size_t p = 0ULL; p < buckets[b].len; p++) {
      object_destroy(&buckets[b].pairs[p].item);
    }
    if (buckets[b].len > 0) {
      iris_free(buckets[b].pairs);
    }
  }
  iris_free(buckets);
}

// todo: just inline in dict_grow?
__forceinline void dict_free_buckets_after_move(IrisDictBucket* buckets, size_t len) {
  for (size_t b = 0ULL; b < len; b++) {
    assert((buckets[b].len > 0 && pointer_is_valid(buckets[b].pairs)) ||
      (buckets[b].len == 0 && !pointer_is_valid(buckets[b].pairs))
    );
    if (buckets[b].len > 0) {
      iris_free(buckets[b].pairs);
    }
  }
  iris_free(buckets);
}

__forceinline void dict_grow(IrisDict* dict) {
  if (dict->card >= (dict->cap - (dict->cap / DICT_GROW_FACTOR))) {
    size_t new_cap = dict->cap << 1ULL;
    IrisDictBucket* new_buckets = iris_alloc0(new_cap, IrisDictBucket);
    for (size_t b = 0; b < dict->cap; b++) {
      for (size_t p = 0; p < dict->buckets[b].len; p++) {
        size_t idx = dict->buckets[b].pairs[p].key % new_cap;
        new_buckets[idx].pairs = iris_resize(new_buckets[idx].pairs, new_buckets[idx].len + 1ULL, IrisDictPair);
        new_buckets[idx].pairs[new_buckets[idx].len] = dict->buckets[b].pairs[p];
        new_buckets[idx].len++;
      }
    }
    dict_free_buckets_after_move(dict->buckets, dict->cap);
    dict->buckets = new_buckets;
    dict->cap = new_cap;
  }
}

static void dict_push(IrisDict* dict, size_t key, IrisObject obj) {
  dict_grow(dict);
  size_t idx = key % dict->cap;
  for (size_t i = 0; i < dict->buckets[idx].len; i++) {
    if (dict->buckets[idx].pairs[i].key == key) {
      object_destroy(&dict->buckets[idx].pairs[i].item);
      dict->buckets[idx].pairs[i].item = obj;
      return;
    }
  }
  dict->buckets[idx].pairs = iris_resize(
    dict->buckets[idx].pairs,
    dict->buckets[idx].len + 1ULL,
    IrisDictPair
  );
  dict->buckets[idx].pairs[dict->buckets[idx].len].item = obj;
  dict->buckets[idx].pairs[dict->buckets[idx].len].key = key;
  dict->buckets[idx].len++;
  dict->card++;
}

void dict_push_object(IrisDict* dict, const IrisObject key, IrisObject* item) {
  assert(dict_is_valid(*dict));
  assert(object_is_valid(key));
  assert(object_is_valid(*item));
  // iris_check(key != item, "can't push item to dict with itself as a key");
  dict_push(dict, object_hash(key), *item);
  // object_move(key);
  object_move(item);
}

void dict_push_string(IrisDict* dict, const IrisObject key, IrisString* str) {
  assert(dict_is_valid(*dict));
  assert(object_is_valid(key));
  assert(string_is_valid(*str));
  dict_push(dict, object_hash(key), string_to_object(*str));
  string_move(str);
}

void dict_push_func(IrisDict* dict, const IrisObject key, IrisFunc* func) {
  assert(dict_is_valid(*dict));
  assert(object_is_valid(key));
  assert(func_is_valid(*func));
  dict_push(dict, object_hash(key), func_to_object(*func));
  func_move(func);
}

void dict_push_list(IrisDict* dict, const IrisObject key, IrisList* list) {
  assert(dict_is_valid(*dict));
  assert(object_is_valid(key));
  assert(list_is_valid(*list));
  dict_push(dict, object_hash(key), list_to_object(*list));
  list_move(list);
}

bool dict_has(const IrisDict dict, const IrisObject key) {
  assert(dict_is_valid(dict));
  assert(object_is_valid(key));
  size_t hash = object_hash(key);
  size_t idx = hash % dict.cap;
  for (size_t i = 0; i < dict.buckets[idx].len; i++) {
    if (dict.buckets[idx].pairs[i].key == hash) {
      return true;
    }
  }
  return false;
}

// todo: should it resize the memory block?
//       it should be okay that bucket forgets its capacity, as it will not attempt to use junk
//       but memory will be freed only on next insertion in the same bucket which is quite random
//       we could probably introduce some switch IRIS_KEEP_MINIMUM_RAM_USAGE for always shrinking memory to minimum
void dict_erase(IrisDict* dict, const IrisObject key) {
  assert(dict_is_valid(*dict));
  assert(object_is_valid(key));
  iris_check(dict_has(*dict, key), "attempt to erase nonexistent key in dict"); // todo: could be inlined into the end of for
  size_t hash = object_hash(key);
  size_t idx = hash % dict->cap;
  for (size_t i = 0; i < dict->buckets[idx].len; i++) {
    if (dict->buckets[idx].pairs[i].key == hash) {
      object_destroy(&dict->buckets[idx].pairs[i].item);
      dict->buckets[idx].pairs[i].item = dict->buckets[idx].pairs[dict->buckets[idx].len - 1ULL].item;
      dict->buckets[idx].len--;
      return;
    }
  }
  __builtin_unreachable();
}

struct _IrisObject dict_get(const IrisDict dict, const IrisObject key) {
  assert(dict_is_valid(dict));
  assert(object_is_valid(key));
  iris_check(dict_has(dict, key), "attempt to get copy of nonexistent key in dict"); // todo: could be inlined into the end of for
  size_t hash = object_hash(key);
  size_t idx = hash % dict.cap;
  for (size_t i = 0; i < dict.buckets[idx].len; i++) {
    if (dict.buckets[idx].pairs[i].key == hash) {
      IrisObject copy = object_copy(dict.buckets[idx].pairs[i].item);
      return copy;
    }
  }
  __builtin_unreachable();
}

const IrisObject* dict_get_view(const IrisDict dict, const IrisObject key) {
  assert(dict_is_valid(dict));
  assert(object_is_valid(key));
  iris_check(dict_has(dict, key), "attempt to get view of nonexistent key in dict"); // todo: could be inlined into the end of for
  size_t hash = object_hash(key);
  size_t idx = hash % dict.cap;
  for (size_t i = 0; i < dict.buckets[idx].len; i++) {
    if (dict.buckets[idx].pairs[i].key == hash) {
      return &dict.buckets[idx].pairs[i].item;
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
  assert(dict_is_valid(*dict));
  dict_free_buckets(dict->buckets, dict->cap);
  dict_move(dict);
}

void dict_move(IrisDict* dict) {
  assert(pointer_is_valid(dict));
  assert(dict_is_valid(*dict));
  dict->buckets = NULL;
  dict->card = 0ULL;
  dict->cap = 0ULL;
}

void dict_print_repr(const IrisDict dict, bool newline) {
  assert(dict_is_valid(dict));
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
