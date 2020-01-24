%{

0 Validate identifier usage
    * Variables must be explicitly initialized.
    * Variables not explicitly initialized are assumed to be functions.
      o This implies that e.g. x{end}, x(:), x(:, end) expressions are
        assumed to be function calls, unless x has been previously defined
        with an initializer.
    * However, scripts complicate things: a script invocation essentially
      acts like a #include directive, pasting the contents of the script
      into the containing scope.
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



Impl.

In a given scope, consider the set of identifier reference expressions and
statements which employ them. An identifier reference expression consists 
of a primary identifier and optional subscripts -- e.g. `a` has no 
subscripts; `a()` has one subscript, with no arguments; `a.b.c` has two 
subscripts, each with one argument. Initially, all identifiers are 
unclassified. Proceed through the set of statements in the current scope. 

There are a few cases in which an identifier can be non-contextually or
locally-contextually classified:
  * If a primary identifier appears on the left hand side of an 
    assignment, or as part of a global or persistent declaration, 
    it must be a variable. 
  * If a primary identifier is the command in a command statement, it must 
    be a function.
  * A primary identifier that is an input or output function parameter must
    be a variable.
  * If a primary identifier is unclassified and a) appears on the right
    hand side of an assignment or as part of an expression statement and b)
    has either no subscripts or a list of subscripts whose first entry
    is a parentheses grouping expression, then it could be a sibling,
    immediate child, or parent function reference. If there is a sibling,
    child, or parent function that matches the identifier, it can be
    classified as such.

If an identifier cannot be locally classified, it may be a function or 
script reference, or else a reference to a variable introduced by a script 
into the current scope. Each remaining unclassified primary identifier must
be considered in sequence. 
  * Each identifier reference expression must be split by considering its 
    subscripts and a "a function call delimiter", i.e. a parentheses 
    subscript. Because variables must be explicitly initialized, a reference 
    expression whose *first subscript is a brace or non-literal field 
    reference expression is an error, e.g. (a{1}, a.('a')). Otherwise, a 
    compound identifier reference expression of the form e.g. a.b.c().d.e() 
    or a.one(1, 2).(two) must contain the functions a.b.c and a.one, 
    respectively, with an optional subscript expression. At this point, if 
    an `end` or `:` subscript appears in the parentheses of a presumed 
    function reference, it is an error.
  * Once split, an identifier must resolve to a function or script
    reference. An interface representing the system path must be used to
    locate the file.
      o A script cannot return outputs or accept inputs, so if a primary
        identifier appears as part of an expression, it cannot refer to a
        script.
      o Issue: functions are dispatched by their arguments. We can't just 
        look up a function by name.
  * The resolved file must be parsed, at least as much as is required to 
    determine what kind of file (i.e., class, function, or script) it 
    represents.
  
If an identifier is used incorrectly, we must classify it as something like
IncorrectVariable. Or else introduce "expected usage" and "actual usage"
fields.

%}