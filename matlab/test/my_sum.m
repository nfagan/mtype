%{

@T begin let
  X = double
  Flag = char | double
  Float = double | single
  Int = uint8 | int8 | uint16 | int16 | uint32 | int32 | uint64 | int64
  Numeric = Float | Int
  Int64 = [int64] = (Numeric)
end

@T begin export
  function another
    [] = ()
  end
end

@T begin  
  given `X` let Y = X
  let X = double
  let Y = single
  let Another = fields S
  let Y = vi<{any}>
  
  given <T, X, Y, Z> function my_sum
    [X | T] = (T, Flag?, Flag?, char?, vi {any...})
  end
end

@T given T function my_sum
  [X | T] = (T, Flag?, Flag?, char?)
end
%}
function s = my_sum(x, dim, type, flag)



end

% @T [int64] = ()
function res = another()

%{
@T begin
  suppose res is int64
  assume a is int64
  assume b is int64
end
%}

res = my_sum( int64(1:10) );
[a, b] = test2();

end

% @T given `T` 
function [a, b, c] = test2()

end