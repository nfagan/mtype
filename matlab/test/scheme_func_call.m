function [doub, str] = scheme_func_call()

% if MT_SCHEME_FUNC_CALL in type_constraint_gen.cpp is (0), then poly2 will
% be monomorphized in poly1.

doub = poly1( [1] );
str = poly1( [""] );

end

% @T fun
function d = poly1(a)
d = poly2( a );
end

% @T fun
function b = poly2(a)
b = a;
end