%{

Function dispatch.

Given an external function named `x`, arguments of type `t0` ... `tn`, 
scope `scope`, and a function application expression of the form 
`x(a: t0, ... z: tn)`:

1. If `x` names a fully-qualified import in `scope`, the function must
resolve to the package function or static method `x`. It is an error if no 
such function can be found.

2. If `x` is not a fully-qualified import in `scope`, consider each
wildcard import in `scope`. If there is a package function or class method 
that matches the combination of a wildcard-imported identifier and `x`,
then the function has been resolved.

3. If there are no matching wildcard imports, check whether the directory 
containing the file associated with `scope` has a folder called `private`. 
If so, check whether there is a function `x` in that folder. If so, the 
function has been resolved.

4. If there is no adjacent private directory or no matching function inside
it, resolution depends on the argument types `t0` ... `tn`:
  * If any argument type is non-concrete, it is not possible to determine
    which function might be called.
  * Otherwise, if all argument types are concrete, check whether `x` is a
    method of types `t0` ... `tn`:
    o If `x` is a method of exactly one type from `t0` ... `tn`, the
      function has been resolved to that method.
    o If `x` is a method of two or more types from `t0` ... `tn`, the
    	resolved method depends on the precedence relationship amongst the
    	defining types. In general, the method is that of the type `ti` with
    	the highest precedence amongst other types in the set, or else that
    	of the left-most argument type, in the case that all types have equal
    	precedence. Note that classes defined with `classdef` have higher
      default precedence than built-in types.

5. If `x` is not a method of any of the concrete argument types `t0` ...
`tn`, check whether there is a class directory '@x' on the search path,
containing a constructor function file 'x.m'. If so, the function has been
resolved.

6. If `x` is not a class constructor in an @-prefixed directory, check 
whether the current working directory contains a function or classdef file 
'x.m'. If so, the function has been resolved.

7. Otherwise, `x` must resolve to the first function file, classdef file,
or built-in function on the path:
  * If `x` names a built-in function, it has been resolved.
  * Otherwise, consider the contents of all directories on the search path
  in order of appearence:
    o If a folder contains a mex file 'x.mex<ext>', the function has been
    resolved.
    o If a folder contains a file 'x.p', the function or class has been
    resolved.
    o If a folder contains a file 'x.m', the function or class has been 
    resolved.

%}