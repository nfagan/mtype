%{

% file1.m

begin export

import file2

% This should be ok -- type parameters live in separate namespace
given Z let Z = Z<Z>
% See below
let X = S

% Should this be allowed?
given T let Self = T
given <X, Y> let Nested = X<Y>

given <A, B> struct AA
  f: Self<A>
  c: Nested<A, B>
end

struct S
  f: X
  y: Z
end

end
%}

function new_scope()

%{
begin

% This `Y` should refer to the parent scope's `X`.
let Y = X

% This `S` should refer to the current scope's `S`.
let A = S

% Defines a recursive struct.
struct S
  f: A | Y
end

end
%}

end

%{

% file2.m

begin export

namespace A
  import file1
end

end

begin export
  import file1

  % Error: X already exists in file1
  let X = X
  let Y = A.S
end

%}

%{

% Circular references
D = double

let A = B
let B = C
let C = A | D

%}

%{

% Aliases
% file1.m
begin export
  import file2 as file2

  class Y
    f1: X
    f2: Y | double
  end

  class X < Y
    f1: Z
    f2: Z2
  end

  given <T> let Value = T | Value<T>

  let JSONValue = char | double | struct_map<char, JSONValue>
  
  let Y = struct(f: double, y: single)
  let Z = struct_map<char, JSONValue>

  let A = file2/file1/A
  let B = file2/B

  % Error: Identifier `A` does not resolve to a type.
  struct S
    f: A
  end
  given T struct ST
    f: T
  end

  % Error: Identifier `B` does not resolve to a type.
  let Y = ST<B>
  let X = Y | file2/C

  % Technically ok, but impossible to instantiate.
  struct S2
    f: S2
    c: double
  end
  % OK: s = struct( 'f', 1, 'c', 2 ); s = struct( 'f', struct('f', 1) );
  struct S3
    f: S3 | double
    c: single
  end
end

% file2.m
begin export
  import file1 as file1

  let B = file1/A
  let C = file1/file2/file1/Y | file1/X
end

%}

%{

Impl.

In a given scope, `let` declarations introduce new variables. It is an 
error for a `let` declaration to redefine an existing variable in the
current scope.

The rhs of a `let` declaration can be a type variable or struct
declaration appearing anywhere in the current scope, or else anywhere in a
parent's scope.

% namespace A
%   begin export
%     namespace B
%       
%     end
%   end
% end

% Parametric type `Y`: 
given <T> struct Y
  f: Y
end
% Concrete type `S`: 
struct S 
  f: double
end
% Parametric type alias `P`: given <T> let P = Y<T>
% Concrete type alias `P`: let P = Y<double>

A given scope has type definitions, type aliases, and namespaces, which
all exist in the same namespace. A namespace does not introduce a new scope, 
but nests declarations under a prefix. 

A file has an export scope which is addressable from any child scope.

Function and classdef files implicitly export the type of the main function
and class, respectively. If the typing scope in which the class or function
is referenced defines a type for that class or function, it must match the
type from the defining file.

Impl.

Create a type reference.

struct TypeScope {
  int64_t uuid;
  ExportList* exports;
  TypeScope* parent;

  std::vector<int64_t> structs;
  std::vector<int64_t> functions;
  std::vector<int64_t> type_aliases;
  std::vector<int64_t> namespaces;
  std::set<int64_t> reserved_identifiers;
}

struct TypeReference {
  int64_t to_definition;
}

struct Field {
  int64_t name;
  int64_t type_ref;
}

struct Exports {
  link_to(
}

struct Import {
  ExportRef* to_export;
};

Start with a given file. Parse it and generate a set of scopes according to
the rules above, checking to ensure no duplicate identifiers are exported
or defined. Mark imports as awaiting resolution. After finishing parsing
the file, consider each marked import. Parse each file associated with a
unique import, recursively. Replace each import with a link to the export 
of the parsed file.

%}

%{

% file1.m

%{
begin
function file1()
  [] = ()
end

function another()
  [double] = ()
end
end
%}
function file1()
  f = another();
end

% --------

% another.m

% @T [] = ()
function another()
end


%}