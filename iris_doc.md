lisp
====
goals:
-----
- be poetic
- typed-by-default and high-performant lisp
- metaprogramming / generalization is key
- optimizable by design
- lists as reversed arrays
- philosophy of dynamic and metamorphic C

- this particular implementation is restricted to typed that are valid
- for .add-64 builtin function, user don't have to specify types unless required
(defn + [l r]
  (.add-64 l r))

- recursions should be inlined as much as possible
(defn fib [n]
  (if (<= n 1)
    n
    (+ (fib (dec n) (fib (- n 2))))))

- customizable reader for extending functionalities
(defn my-reader [string]
  "receives string and parses it to list of code data"
  (pass))
(!set-reader my-reader)
