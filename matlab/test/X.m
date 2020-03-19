function [x0, x1, x2, x3] = X(z1) 

a0 = {1, fileparts('c', 1)};
a3 = {min(min(1, ""), ""), min(1, ""), 1, 'c'};
a1 = {min(1, ""), 'c'};
% a2 = a1;
a2 = a3;
[a2{1}] = a3{1};

double_list = {1, 1, 1, min(1, ""), 1};
z0 = {1, double_list{1}, min(1, "")};
y = in_list( 1, 1, 1, 2, 3, 4, z1{1} );
z1{1} = z0{1};
z1 = z0;

[~, x0, x1] = lists( 1 );

z2 = {min(1, ""), 2};
z3 = z2;
[z2{1}] = min( 1, "" );
[z2{1}, z3{1}] = double_list{1};
x3 = z3{1};
x2 = z2;

% xx = zz@(a, b, c) 1;
% if (true )
% end
% xx = zz@(a, b, c) 1;

% [x2, x3] = list_dt_out( 1 );
  
end