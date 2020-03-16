function [a, b, c, d, x, y, z] = X()

c = 1;
aa = c(c, c, c, c);

d = c(c(c), min(min(c, "c"), "c"), c(c(c)));
z = min( min(1, "c"), "c" );

y = 1;
[a, x, y] = lists( y );

% b = {a, 1, min(c, "c")};
b = {a, 1, min(c, "c")};
zz = b{c};

another = aa(1);

end

% function aa = another(y)
% 
% a = 1;
% [xx, yy] = min( a, "c" );
% 
% [aa(aa), x, y] = lists( a );
% y = aa;
% 
% b = 2;
% [a{:}, b{:}] = match_lists( b, a );
% 
% % aa = 'c';
% 
% % xx = 'c';
% 
% % func( func2() );  % r-value tuple containing function outputs
% % 
% 
% end