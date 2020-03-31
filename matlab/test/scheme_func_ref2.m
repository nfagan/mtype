function f = scheme_func_ref2()

% If MT_SCHEME_FUNC_REF is (1) and MT_SCHEME_FUNC_CALL is (1) 
% in type_constraint_gen.cpp, then the initial invocation of one(1) does
% not propagate double as the argument type to @two and @three.

one( 1 );

f = @one;
f = @two;
f = @three;
f = @sum;

% accept( f );
accept( @test1 );

end

% @T fun
function accept(t)
t( 1 );
end

function test1(a)
end

function [yx, y3, yy] = one(yu)

rec = @(x) x(1) + 5;
t = @(x, y) x + y(x);
u = @(x) t(x, @sum);
v = @(x) t(x, @(x) x + 1);
id = @(x) x;
y0 = @(t) rec(u) + 2 + t;
y1 = @(t) rec(v) + t + y0(1);
y2 = @(o) id(rec(u) + rec(v) + rec(u) + rec(v) + rec(u) + o) + y1(1);
y3 = rec( y2 );
y4 = @(x) {y2(1), rec(@(o) id(rec(u) + rec(v) + rec(u) + rec(v) + rec(u) + o))};
y6 = @(x) y4(x);
y5 = y4(1);
y7 = y6(1);
yx = [y5{1}, 1, y7{1}];
yy = @(x) sum(x);
yy( yu );

end

function [a, b, c] = two(x)

a = 1;
b = 1;
c = @(z) x;

end

function [e, f, g] = three(x)

e = feval( @(x) x, x );
f = feval( @(x) x, x );
g = feval( @(x) @(z) feval(x, 1), @(z) x );

end