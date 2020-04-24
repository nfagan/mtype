%{

TODO

. global / persistent variables, command statements not handled.
. * ternary colon operator (1:2:3)
. * spmd and parfor statements
. single-line if statements (if 1 true; else false)
. * static method lookup; wildcard imports; proper method dispatch
. * proper scheme simplification / relation
. For method definition, use provided type as canonical type
. * Distinguish application and abstraction
. Distinguish concatenation types - horz vs. vert
. union types
. recursive types
. * error variable in catch expression
. * An input list<T> -- that is, dt-i[list<T>] -- should match e.g.
        dt-r[list<double>, double]
    or  dt-r[list<double>, double, list<double>], etc.

    e = {1, 2, 3};
    u = [e{1}, 1];

%}
