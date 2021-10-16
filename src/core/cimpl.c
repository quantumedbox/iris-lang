#include "eval.h"

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdnoreturn.h>
#include <stdlib.h>
#include <time.h>

#include "types/types.h"
#include "memory.h"
#include "utils.h"

// todo: they're bodged as hell, should standardize the way they're implemented

/*
  @brief    Yields passed argument as it is, required for escaping evaluation
  @variants (1: any)
*/
static IrisObject quote(const IrisObject* args, size_t arg_count) {
  if (arg_count != 1ULL) {
    panic("invalid argument count for quote"); // should it not hard crush, but return error result?
  }
  return args[0]; // None
}

/*
  @brief    Exits the application
  @variants (0) (1: int)
*/
noreturn static IrisObject quit(const IrisObject* args, size_t arg_count) {
  if (arg_count > 1ULL) {
    panic("invalid argument count for exit"); // should it not hard crush, but return error result?
  } else if (arg_count == 0ULL) {
    exit(0);
  } else {
    iris_check(args[0].kind == irisObjectKindInt, "not int object passed to exit");
    exit(args[0].int_variant);
  }
}

// In-Iris alternative:
// >>> (defn echo [& args]
//        (apply print args))
/*
  @brief    Prints repr of objects into stdout
  @variants (n: any)
*/
static IrisObject echo(const IrisObject* args, size_t arg_count) {
  assert(((arg_count != 0ULL) && pointer_is_valid(args)) || (arg_count == 0));
  for (size_t i = 0ULL; i < arg_count; i++) {
    if (i != 0ULL) { (void)fputc(' ', stdout); }
    object_print_repr(args[i], false);
  }
  (void)fputc('\n', stdout); // todo: it shouldn't print newline on empty seq?
  return (IrisObject){0}; // None
}

// todo: it may leak memory if body tries to return allocated object
/*
  @brief    Macro for evaluating body n times, results of evaluations are dropped, returns None
  @variants (2: int body)
*/
static IrisObject repeat_eval(const IrisObject* args, size_t arg_count) {
  assert(pointer_is_valid(args));
  if (arg_count != 2ULL) {
    panic("invalid argument count for repeat");
  } else if (args[0].kind != irisObjectKindInt) {
    panic("first argument of repeat isn't int");
  }
  const IrisDict* scope = get_standard_scope_view();
  for (int i = 0; i < args[0].int_variant; i++) {
    (void)eval_object(args[1], scope);
  }
  return (IrisObject){0}; // None
}

/*
  In-Iris alternative:
  >>> (defn-macro timeit [arg]
        (let [start (cputime)
              result (arg)]
          (echo "elapsed: " (- start (cputime)))
          result))
*/

// todo: should return result instead, time could be just printed
/*
  @brief    Macro for timing the execution of body
  @return   Float -- seconds of execution
  @variants (1: body)
*/
static IrisObject timeit(const IrisObject* args, size_t arg_count) {
  if (arg_count != 1ULL) {
    panic("timeit should have only one argument");
  }
  assert(pointer_is_valid(args));
  assert(object_is_valid(args[0]));

  const IrisDict* scope = get_standard_scope_view();
  clock_t start_time = clock();
  (void)eval_object(args[0], scope);
  clock_t end_time = clock();
  IrisObject result = { .kind = irisObjectKindFloat, .float_variant = (float)(end_time - start_time) / CLOCKS_PER_SEC };
  return result;
}

// In-Iris alternative:
// >>> (defn-macro reduce [fn ls]
//        (type-guard [fn func-t
//                     ls eval-t]
//          (let ))) ; todo
// todo: what a mess
/*
  @brief    Apply function to arguments with carry
  @return   Result of last function call
  @variants (2: func list)
*/
static IrisObject reduce(const IrisObject* args, size_t arg_count) {
  iris_check(arg_count == 2ULL, "invalid args count for reduce macro");
  assert(pointer_is_valid(args));
  assert(object_is_valid(args[0]));
  assert(object_is_valid(args[1]));
  iris_check(args[0].kind == irisObjectKindString, "first argument of reduce should be symbol");
  const IrisDict* scope = get_standard_scope_view();
  IrisObject supposedly_list = eval_object(args[1], scope);
  iris_check(supposedly_list.kind == irisObjectKindList, "second argument of reduce should be list");
  iris_check(supposedly_list.list_variant.len >= 2ULL, "there should be at least 2 arguments in list to be reduced");
  const IrisObject* supposedly_callable = dict_get_view(scope, args[0].string_variant.hash);
  iris_check(supposedly_callable->kind == irisObjectKindFunc, "looked up object isn't callable");
  IrisObject cell[2] = {
    func_call(supposedly_callable->func_variant, &supposedly_list.list_variant.items[0], 2ULL),
    (IrisObject){0}
  };
  for (size_t i = 2ULL; i < supposedly_list.list_variant.len; i++) {
    cell[1] = supposedly_list.list_variant.items[i];
    cell[0] = func_call(supposedly_callable->func_variant, cell, 2ULL);
  }
  return cell[0];
}

/*
  @brief    Addition operation
            Promotes integers to floats if one of arg is float
  @return   Float | Int
  @variants (2: (float | int) (float | int))
*/
static IrisObject plus(const IrisObject* args, size_t arg_count) {
  iris_check(arg_count == 2ULL, "invalid args count for plus operation");
  assert(pointer_is_valid(args));
  iris_check((args[0].kind == irisObjectKindInt) || (args[0].kind == irisObjectKindFloat), "invalid type for plus operation");
  iris_check((args[1].kind == irisObjectKindInt) || (args[1].kind == irisObjectKindFloat), "invalid type for plus operation");
  IrisObject result = {0};
  if (args[0].kind != args[1].kind) {
    result = (IrisObject){
      .kind = irisObjectKindFloat,
      .float_variant =
        ((args[0].kind == irisObjectKindInt) ? (float)args[0].int_variant : args[0].float_variant) +
        ((args[1].kind == irisObjectKindInt) ? (float)args[1].int_variant : args[1].float_variant)
    };
  } else {
    result = (IrisObject){
      .kind = args[0].kind,
      .int_variant =
        ((args[0].kind == irisObjectKindInt) ? args[0].int_variant : args[0].float_variant) +
        ((args[1].kind == irisObjectKindInt) ? args[1].int_variant : args[1].float_variant)
    };
  }
  return result;
}
