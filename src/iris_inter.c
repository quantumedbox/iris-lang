#include <pthread.h>
#include <locale.h>
#include <assert.h>

#include "iris_inter.h"
#include "iris_eval.h"
#include "types/iris_types.h"
#include "iris_memory.h"
#include "iris_utils.h"

typedef struct _IrisInterThread {
  // handle on which interpreter caller can interact with started interpreter instance
  pthread_t thread;
} IrisInterThread;

typedef struct {
  IrisList inhereted_scopes;  // immutable formed scopes
  IrisDict local_scope;       // interpreter-local scope that could be modified
} IrisInter;

typedef struct {
  IrisList codelist;
} IrisInterThreadPayload;

IrisInterThread* inter_new(void) {
  return iris_alloc0(1, IrisInterThread);
}

void inter_destroy(IrisInterThread** handle) {
  assert(handle != NULL);
  assert(*handle != NULL);
  iris_free(*handle);
  *handle = NULL;
}

static void inter_eval_thread_init(void) {
  // todo: not sure about that, might it fuck with calling thread?
  // todo: posix locale when available
  _configthreadlocale(_ENABLE_PER_THREAD_LOCALE);
  setlocale(LC_ALL, ".utf8");
}

static void* inter_eval_thread(void* payload_void) {
  inter_eval_thread_init();
  IrisInterThreadPayload payload = *(IrisInterThreadPayload*)payload_void;
  assert(list_is_valid(payload.codelist));
  IrisObject* result = iris_alloc0(1, IrisObject);
  *result = eval_codelist(payload.codelist);
  list_destroy(&payload.codelist);
  iris_free(payload_void);
  pthread_exit((void*)result);
  return NULL;
}

bool inter_eval_codelist(IrisInterThread** handle, IrisList* codelist) {
  IrisInterThreadPayload* payload = iris_alloc0(1, IrisInterThreadPayload);
  payload->codelist = *codelist;
  int err = pthread_create(&((*handle)->thread), NULL, &inter_eval_thread, payload);
  if (err != 0) {
    list_destroy(codelist);
    return false;
  } else {
    list_move(codelist);
    return true;
  }
}

IrisObject inter_result(IrisInterThread** handle) {
  IrisObject* buffer = NULL;
  int err = pthread_join((*handle)->thread, (void**)&buffer);
  IrisObject result = *buffer;
  iris_free(buffer);
  iris_check(err == 0, "error on interpreter thread joining");
  iris_check(object_is_valid(result), "ill-formed interpreter thread return");
  return result;
}
