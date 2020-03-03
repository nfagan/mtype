%{

1. Subclass method definition or redefinition
  * If a subclass overrides a superclass method, the subclass method
    signature must be subtype related to that of the superclass. In
    particular, given class T, subclass S, and method M, the number of 
    inputs and outputs must be the same between T and S's implementation of 
    M; for each input argument Mi, Mi(S) > Mi(T); for each output argument
    Mo, Mo(S) < Mo(T).

%}