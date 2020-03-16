function [a, b, c, d, x, y] = X(a)

% a = 1;
% b = 2;
% a = b(1, 2, 3, a);

% a = 1;
% b = 'c';
% 
% a = 1;
% c = 1;
c = 1;

% aa = c(c, c, c, c);

% d = c(c(c), min(min(c, "c"), "c"), c(c(c)));

y = 1;
[a, x, y] = lists( y );

end

function aa = another(y)

a = 1;
[xx, yy] = min( a, "c" );

[aa(aa), x, y] = lists( a );
y = aa;

b = 2;
[a{:}, b{:}] = match_lists( b{:}, a{:} );

% aa = 'c';

% xx = 'c';

% func( func2() );  % r-value tuple containing function outputs
% 

end