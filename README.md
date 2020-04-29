# mtype

`mtype` presents a type language and type checker for MATLAB code.

Type annotations live in the comments of otherwise standard MATLAB code. Here's a short example:

```MATLAB
%  my_typed_add.m
%  @T import mt.base
%  @T :: [double] = (double, double)
function c = my_typed_add(a, b)

c = a + b;

end
```

```MATLAB
%  use_typed_add.m
function a = use_typed_add()

a = my_typed_add( 1, 2 );
b = my_typed_add( '', 2 ); % error. 

end
```

Run the type checker at the command line:
`mtype use_typed_add -pf /path/to/path/file.txt`

## features

* Monomorphic + polymorphic function types.
* Monomorphic class types (todo: polymorphic class types).
* Monomorphic + polymorphic named `record` types.
* Imports and exports; namespaces; type aliases.
* Tuples; unions; lists.
* Obeys MATLAB function dispatch.
* Enforces subtype relationships.

## issues / todo
* Inference is performed naively; its very slow for non-trivial programs.
* Polymorphic class types (soon).
* Recursive types.
* `events` and `enum` constructs in class definitions.
* Windows support.
* More tests.

## build

`mtype` builds on macOS and linux, and requires a compiler with support for c++17. The build system is cmake. Assuming a valid default compiler, clone this repository, `cd` into it, and run the following to make a release build:
```sh
mkdir build
cd ./build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target mtype --config Release
```

## usage

```
Usage: mtype [options] file-name [file-name2 ...]

options: 
  --help, -h: 
      Show this text.
  --show-ast, -sa: 
      Print each file's AST.
  --show-var-types, -sv: 
      Print the types of local variables.
  --matlab-function-types, -mft: 
      Print function types using the form: [] = ().
  --arrow-function-types, -aft: 
      Print function types using the form: () -> [].
  --show-visited-files, -svf: 
      Print the paths of traversed files.
  --show-class-source, -scs: 
      Print the source type for class types.
  --show-dist, -sd: 
      Print the distribution of types.
  --explicit-dt, -edt: 
      Print destructured tuples explicitly.
  --explicit-aliases, -ea: 
      Print aliases explicitly.
  --plain-text, -pt: 
      Use ASCII text and no control characters when printing.
  --hide-function-types, -hf: 
      Don't print local function types.
  --show-function-types, -sf: 
      Print local function types.
  --hide-diagnostics, -hdi: 
      Don't print elapsed time and other diagnostics.
  --show-diagnostics, -sdi: 
      Print elapsed time and other diagnostics.
  --hide-errors, -he: 
      Don't print errors.
  --hide-warnings, -hw: 
      Don't print warnings.
  --path, -p `str`: 
      Use directories in the ':'-delimited `str` to build the search path.
  --path-file, -pf `file`: 
      Use the newline-delimited directories in `file` to build the search path.
```


