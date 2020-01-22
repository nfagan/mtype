%{

0 Validate identifier usage
    * Consider a function `f`. Establish the names of sibling and children
      functions of `f`. Identifiers in 
1 Disambiguate variable references from function call expressions.
    * An identifier that is brace-referenced (`a{}`) cannot be a function.
    * An identifier reference expression with no subscripts whose main
      identifier is a function input or output parameter cannot be a
      function.
    * An identifier reference expression with multiple literal field
      reference expressions in a row may or may not represent a package /
      class method call. For example, in `a.b.c().d{1};` `a.b.c` could be a
      function or a nested struct.
    * In a command statement, the command identifier must be a function.
    * An identifier that is the target of assignment cannot be a function.
      o This implies that a nested function cannot share the name of an
        assignment target in the current scope.
2 For function call expressions, verify that the function exists, and
  obtain its type signature.
    * Obtaining the type signature will require parsing the associated
    file.
3  

%}