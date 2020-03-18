function [z0, bb, x, z2, y, a1, a0] = X()

z0 = {1, min(1, "c"), lists(1)};
bb = {1};
b = {1, 'c', 2};
a = 3;
[x] = a(1);
z = {a, min(x, "c"), a, 1};

z2 = z0;

[z0{min(x, "c"), 1}] = z{a};
x(1) = z{1};
[y, y0, y1, y2] = z{1};

a0 = {1, fileparts('c', 1)};
a1 = {min(1, "c"), 'c'};
a2 = a1;

[a1{1}, a2{1}] = a0{1};
% a1 = a0;

% [a1{1, 2, 3}] = a0{1, 2, 3};

end