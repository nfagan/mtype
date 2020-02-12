%{

constraint Scalar(T) := size(value(T)) == [1, 1]
constraint Positive(T) := all(value(T) > 0)
constraint ScalarPositive(T) := Scalar(T) && Positive(T)
constraint DotSubscriptable(T) := T <: Record

given <X, T where Scalar, U where Scalar> function [X] = constraints(T, U)

%}

function x = constraints(a, b)
% constraint: tx = t(ta + tb);
x = a + b;

function child()
  % observation: size(x) = size( [a, b] );
  % constraint: tx = t([ta, tb]);
  x = [a, b];
end

% constraint: tx = t(ta * tb)
x = x * b;
end