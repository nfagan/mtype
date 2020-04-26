% @T import mt.base
function [c, e] = scheme_func_ref1()

% If MT_SCHEME_FUNC_REF is (0) and MT_SCHEME_FUNC_CALL is (1) 
% in type_constraint_gen.cpp, then the function references in 
% deal_poly() are erroneously monomorphized.

[a, b] = deal_poly();
[g, h] = deal_poly();

c = a( 1 );
e = b( "" );

c1 = g( "" );
e1 = h( 1 );

u = @sum;
uu = u( 1 );

end

% @T fun
function d = poly1(a)
d = poly2( a );
end

% @T fun
function b = poly2(a)
b = a;
end

% @T fun
function [o, u] = deal_poly()
% o = @poly1;
% u = @poly2;
o = @(x) poly1(x);
u = @(x) poly2(x);
end