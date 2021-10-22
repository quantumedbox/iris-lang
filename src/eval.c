// todo: evaluation of "quote" lists returned from other lists
//       maybe it could some sort of "delayed" promise? that should be computed on evaluation

// todo: (help)
// todo: (doc <symbol>)
// todo: (for [binding list [binding list ...]] body) -- iteration over list

#include "core/cimpl.c"

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <signal.h>
#include <stdlib.h>

#include "eval.h"
#include "types/types.h"
#include "reader.h"
#include "memory.h"
#include "utils.h"
#include "iris.h"

#define IRIS_ARGUMENT_STACK_LIMIT 32

const char* repl_welcome_msg =
  "- Iris REPL -\n"
  "| version -- "IRIS_VERSION"\n"
  "| compiled -- "__DATE__"\n"
  "| enter (help) or (doc <name>) for getting info\n"
  "| ctrl+c or (quit) for exit\n";

static IrisDict standard_scope; // todo: make it as RefCell of Dict? we would need it if we want to go full "meta" and have ability to retrieve it

static volatile bool repl_should_exit = false; // todo: shouldn't be here

IrisDict scope_default(IrisList argv_list) {
  IrisDict result = dict_new();
  #define push_to_scope(m_push_by, m_cfunc, m_symbol) {         \
    IrisFunc func = m_push_by(m_cfunc);                         \
    IrisString symbol = string_from_chars(m_symbol);            \
    dict_push_func(&result, string_to_object(symbol), &func);   \
    string_destroy(&symbol);                                    \
  }
  // argv list
  IrisString argv_name = string_from_chars("argv"); // todo: kinda lame that we have to create strings for that, there should be way to hash chars
  IrisObject argv_list_obj = list_to_object(argv_list);
  IrisObject argv_list_shared = refcell_to_object(refcell_from_object(&argv_list_obj));
  dict_push_object(&result, string_to_object(argv_name), &argv_list_shared);
  string_destroy(&argv_name);

  // functions
  push_to_scope(func_from_cfunc,        cimpl_eval,         "eval");
  push_to_scope(func_from_cfunc,        cimpl_nurture,      "nurture");
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
  push_to_scope(func_from_cfunc,        cimpl_reduce,       "reduce");
  push_to_scope(func_macro_from_cfunc,  cimpl_timeit,       "timeit!");
  push_to_scope(func_macro_from_cfunc,  cimpl_repeat_eval,  "repeat-eval!");
  push_to_scope(func_from_cfunc,        cimpl_metrics,      "metrics");

  return result;
  #undef push_to_scope
}

void eval_module_init(int argc, const char* argv[]) {
  if (dict_is_valid(standard_scope)) {
    dict_destroy(&standard_scope);
    warning("reconstruction of default scope dictionary");
  }
  IrisList argv_list = list_new();
  for (int i = 0; i < argc; i++) {
    IrisString str = string_from_chars(argv[i]);
    assert(string_is_valid(str));
    list_push_string(&argv_list, &str);
  }
  standard_scope = scope_default(argv_list);
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
  (void)fprintf(stdout, "%s", repl_welcome_msg);
  while (!repl_should_exit) {
    (void)fprintf(stdout, ">>> ");
    IrisString line = string_from_file_line(stdin);
    IrisObject code = string_read(line);
    IrisObject run = codelist_resolve(code, *scope);
    // IrisObject result = eval_list(sprout, scope);
    // object_print_repr(result, true);
    object_print_repr(run, true);
    // object_destroy(&result);
    string_destroy(&line);
    object_destroy(&code);
    object_destroy(&run);
  }
  signal(SIGINT, SIG_DFL);
}

// todo: define ways of scope modification
//       it could probably be done by special dicts that have back references to scope from which they inherit
// todo: there should be no symbol lookup on evaluation
IrisObject eval_object(const IrisObject obj, const IrisDict* scope) {
  assert(obj.kind < N_OBJECT_KINDS && obj.kind >= 0);
  if ((obj.kind == irisObjectKindList) && (obj.list_variant.len > 0ULL)) {
    // non-empty lists are evaluated, you need to place them in quote list to get them as they are
    assert(object_is_valid(obj.list_variant.items[0]));
    IrisObject callable = eval_object(obj.list_variant.items[0], scope);
    assert(object_is_valid(callable));
    if (callable.kind == irisObjectKindError) {
      // errors returned immediately
      return callable;
    } else if (callable.kind != irisObjectKindFunc) {
      // leading item should be of func type
      return error_to_object(error_from_chars(irisErrorTypeError, "can't call non-function object"));
    }
    if (callable.func_variant.is_macro) {
      // if function is macro - don't evaluate its parameters and pass as is
      IrisObject result = func_call(callable.func_variant, &obj.list_variant.items[1], obj.list_variant.len - 1ULL);
      object_destroy(&callable);
      return result;
    } else {
      // otherwise evaluate the parameters
      IrisObject argument_stack[IRIS_ARGUMENT_STACK_LIMIT];
      if (obj.list_variant.len >= IRIS_ARGUMENT_STACK_LIMIT) {
        return error_to_object(error_from_chars(irisErrorStackError, "argument stack exceeded"));
      }
      for (size_t i = 0ULL; i < obj.list_variant.len - 1ULL; i++) {
        assert(object_is_valid(obj.list_variant.items[i + 1ULL]));
        argument_stack[i] = eval_object(obj.list_variant.items[i + 1ULL], scope);
        assert(object_is_valid(argument_stack[i]));
        if (argument_stack[i].kind == irisObjectKindError) {
          for (size_t free_i = 0ULL; free_i < i; free_i++) {
            object_destroy(&argument_stack[free_i]);
          }
          return argument_stack[i];
        }
      }
      IrisObject result = func_call(
        callable.func_variant,
        obj.list_variant.len > 1 ? &argument_stack[0] : NULL,
        obj.list_variant.len - 1ULL);
      object_destroy(&callable);
      for (size_t free_i = 0ULL; free_i < obj.list_variant.len - 1ULL; free_i++) {
        object_destroy(&argument_stack[free_i]);
      }
      return result;
    }
  } else if (obj.kind == irisObjectKindString) {
    // strings are used for lookup
    assert(string_is_valid(obj.string_variant));
    if (dict_has(*scope, obj)) {
      IrisObject lookup = dict_get(*scope, obj);
      assert(object_is_valid(lookup));
      return lookup;
    } else {
      return error_to_object(error_from_chars(irisErrorNameError, "unknown symbol"));
    }
  } else {
    // can only return copies as ownership of object is on caller
    IrisObject copy = object_copy(obj);
    return copy;
  }
}

IrisObject eval_list(const IrisList list, const IrisDict* scope) {
  assert(list_is_valid(list) && dict_is_valid(*scope));
  if (list.len == 0ULL) {
    return (IrisObject){0};
  }
  for (size_t i = 0ULL; i < (list.len - 1ULL); i++) {
    IrisObject result = eval_object(list.items[i], scope);
    if (result.kind == irisObjectKindError) {
      return result;
    }
    object_destroy(&result);
  }
  return eval_object(list.items[list.len - 1ULL], scope);
}
