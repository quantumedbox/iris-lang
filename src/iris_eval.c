#include "core/cimpl.c"

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "iris_eval.h"
#include "types/iris_types.h"
#include "iris_inter.h"
#include "iris_reader.h"
#include "iris_memory.h"
#include "iris_utils.h"
#include "iris_misc.h"

// todo: evaluation of "quote" lists returned from other lists
//       maybe it could some sort of "delayed" promise? that should be computed on evaluation

// todo: (help)
// todo: (doc <symbol>)
// todo: (for [binding list [binding list ...]] body) -- iteration over list
// todo: call stack for debugging

#define IRIS_ARGUMENT_STACK_LIMIT 32

const char* repl_welcome_msg =
  "- Iris REPL -\n"
  "| version -- "IRIS_VERSION"\n"
  "| compiled -- "__DATE__"\n"
  "| enter (help) or (doc <name>) for getting info\n"
  "| ctrl+c or (quit) for exit\n";

static IrisDict standard_scope; // todo: make it as RefCell of Dict? we would need it if we want to go full "meta" and have ability to retrieve it

static volatile bool repl_should_exit = false; // todo: shouldn't be here

IrisDict scope_default(void) {
  IrisDict result = dict_new();
  #define push_to_scope(m_push_by, m_cfunc, m_symbol) {         \
    IrisFunc func = m_push_by(m_cfunc);                         \
    IrisString symbol = string_from_chars(m_symbol);            \
    dict_push_func(&result, string_to_object(symbol), &func);   \
    string_destroy(&symbol);                                    \
  }

  // todo: decouple from cimpl.c
  // functions
  // push_to_scope(func_from_cfunc,        cimpl_eval,         "eval");
  // push_to_scope(func_from_cfunc,        cimpl_nurture,      "nurture");
  push_to_scope(func_macro_from_cfunc,  cimpl_quote,        "quote!");
  push_to_scope(func_from_cfunc,        cimpl_echo,         "echo");
  // push_to_scope(func_from_cfunc,     cimpl_scope,        "scope"); // todo
  // push_to_scope(func_from_cfunc,     cimpl_def,          "def"); // todo
  // push_to_scope(func_from_cfunc,     cimpl_defn,         "defn"); // todo
  // push_to_scope(func_from_cfunc,     cimpl_defmacro,     "defmacro"); // todo
  push_to_scope(func_from_cfunc,        cimpl_quit,         "quit");
  push_to_scope(func_from_cfunc,        cimpl_first,        "first");
  push_to_scope(func_from_cfunc,        cimpl_rest,         "rest");
  push_to_scope(func_from_cfunc,        cimpl_add,          "+");
  push_to_scope(func_from_cfunc,        cimpl_sub,          "-");
  // push_to_scope(func_from_cfunc,        cimpl_reduce,       "reduce");
  // push_to_scope(func_macro_from_cfunc,  cimpl_timeit,       "timeit!");
  // push_to_scope(func_macro_from_cfunc,  cimpl_repeat_eval,  "repeat-eval!");
  push_to_scope(func_from_cfunc,        cimpl_metrics,      "metrics");

  return result;
  #undef push_to_scope
}

void eval_module_init(void) {
  if (dict_is_valid(standard_scope)) {
    dict_destroy(&standard_scope);
    warning("reconstruction of default scope dictionary");
  }
  standard_scope = scope_default();
}

void eval_module_deinit(void) {
  if (dict_is_valid(standard_scope)) {
    dict_destroy(&standard_scope);
  } else {
    panic("default scope dictionary is ill-formed");
  }
}

const IrisDict* get_standard_scope_view(void) {
  assert(dict_is_valid(standard_scope));
  return &standard_scope;
}

static void user_interrupt_handler(int sig) {
  (void)sig;
  repl_should_exit = true;
}

// todo: repl stack that allows nesting
//       only one interpreter instance should be active in repl mode at the same time
void enter_repl(void) {
  if (signal(SIGINT, user_interrupt_handler) == SIG_ERR) {
    iris_check_warn(true, "problem with setting up SIGINT handler for repl");
  }
  const IrisDict* scope = get_standard_scope_view();
  (void)fputs(repl_welcome_msg, stdout);
  while (!repl_should_exit) {
    (void)fputs(">>> ", stdout);
    IrisString line = string_from_file_line(stdin);
    IrisObject code = string_read(line);
    IrisObject torun = codelist_resolve(code, *scope);
    if (torun.kind != irisObjectKindError) {
      IrisObject result = eval_codelist(torun.list_variant);
      object_print_repr(result, true);
      object_destroy(&result);
    } else {
      object_print_repr(torun, true);
    }
    string_destroy(&line);
    object_destroy(&code);
    object_destroy(&torun);
  }
  signal(SIGINT, SIG_DFL);
}

void eval_file(const IrisString filename) {
  iris_check(filename.len <= PATH_MAX, "filename length exceeded system's limit");
  char path[filename.len + 1ULL];
  memcpy(path, filename.data, filename.len * sizeof(char));
  path[filename.len] = '\0';
  FILE* file;
  file = fopen(path, "rb");
  iris_check(file != NULL, "cannot open file for evaluation");
  IrisString content = string_from_file(file);
  fclose(file);
  const IrisDict* scope = get_standard_scope_view();
  IrisObject code = string_read(content);
  if (code.kind != irisObjectKindError) {
    IrisObject torun = codelist_resolve(code, *scope);
    if (torun.kind != irisObjectKindError) {
      IrisInterHandle inter = inter_new();
      if (inter_eval_codelist(&inter, &torun.list_variant) == false) {
        panic("couldn't start interpreter");
      }
      IrisObject result = inter_result(&inter);
      inter_destroy(&inter);
      if (result.kind == irisObjectKindError) {
        (void)fputs(ANSI_ESCAPE_ERROR"evaluation error:"ANSI_ESCAPE_RESET" ", stderr);
        object_print_repr(result, true);
      }
      object_destroy(&result);
    } else {
      (void)fputs(ANSI_ESCAPE_ERROR"resolving error:"ANSI_ESCAPE_RESET" ", stderr);
      object_print_repr(torun, true);
    }
  } else {
    (void)fputs(ANSI_ESCAPE_ERROR"reader error:"ANSI_ESCAPE_RESET" ", stderr);
    object_print_repr(code, true);
  }
  object_destroy(&code);
  string_destroy(&content);
}

// todo: define ways of scope modification
//       it could probably be done by special dicts that have back references to scope from which they inherit
IrisObject eval_object(const IrisObject obj) {
  assert(object_is_valid(obj));
  if ((obj.kind == irisObjectKindList) &&
      (obj.list_variant.len > 0ULL) &&
      (obj.list_variant.items[0].kind == irisObjectKindFunc)) {
    iris_check((obj.list_variant.len - 1ULL) <= IRIS_ARGUMENT_STACK_LIMIT, "argument stack exceeded");
    IrisObject arguments[obj.list_variant.len - 1ULL];
    for (size_t i = 1ULL; i < obj.list_variant.len; i++) {
      IrisObject evaluated = eval_object(obj.list_variant.items[i]);
      if (evaluated.kind == irisObjectKindError) {
        for (size_t d = 1ULL; d < i; d++) {
          object_destroy(&arguments[d - 1ULL]);
        }
        return evaluated;
      }
      arguments[i - 1ULL] = evaluated;
    }
    // todo: what if leading object is not func object but func list? which resolves to a function to be called
    IrisObject result = func_call(obj.list_variant.items[0].func_variant, arguments, obj.list_variant.len - 1ULL);
    for (size_t d = 0ULL; d < obj.list_variant.len - 1ULL; d++) {
      object_destroy(&arguments[d]);
    }
    return result;
  }
  return object_copy(obj);
}

IrisObject eval_codelist(const IrisList list) {
  assert(list_is_valid(list));
  if (list.len == 0ULL) {
    return (IrisObject){0}; // nil
  }
  for (size_t i = 0ULL; i < (list.len - 1ULL); i++) {
    IrisObject result = eval_object(list.items[i]);
    if (result.kind == irisObjectKindError) {
      return result;
    }
    object_destroy(&result);
  }
  return eval_object(list.items[list.len - 1ULL]);
}
