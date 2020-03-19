function [y, y1, x0, x1, x2, x3, d, a] = X(z1) 

a0 = {1, fileparts('c', 1)};
a3 = {min(min(1, ""), ""), min(1, ""), 1, 'c'};
a1 = {min(1, ""), 'c'};
% a2 = a1;
a2 = a3;
[a2{1}] = a3{1};

double_list = {1, 1, 1, min(1, ""), 1};
z0 = {1, double_list{1}, min(1, "")};
z1{1} = z0{1};
z1 = z0;

[~, x0, x1] = lists( 1 );

z2 = {min(1, ""), 2};
z3 = z2;
[z2{1}] = min( 1, "" );
[z2{1}, z3{1}] = double_list{1};
[x3, x4] = z3{1};
x2 = z2;

y = in_list( 1, 1, 1, 2, 3, 4, z1{1} );
y1 = in_list( min(1, "c"), out_list(1), 1, 1 );

d = {1};
a = {1};

% xx = zz@(a, b, c) 1;
% if (true )
% end
% xx = zz@(a, b, c) 1;

% [x2, x3] = list_dt_out( 1 );

% In an assignment a = b, where b resolves to a list<T>, and a is a variable,
% a resolves to T.
  
end