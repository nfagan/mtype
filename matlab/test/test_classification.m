function y = test_classification(y)

% import y.keep;
%{
@T begin

let X = [] = (double | single)
given T let Rec = X<T>
given T let Self = Rec<T> | T

end
%}

y = test_classification;
y = z();

a = keep( y );
x = keep( keep() );

x = disp( 1 );
y = disp( 2 );

  function nest
    import y.keep;
    x = keep();
    y = keep();
    z = keep();
    
    function two
      y = keep;
    end
  end

end

% @T double | single
function [] = another()

%{
begin export

import file2

% This should be ok -- type parameters live in separate namespace
given Z let Z = Z<Z>
let Z = y
% See below
let X = S

struct S
  f: X
  y: Z
end

end
%}

x = y;

end

%{

begin export
  import file1

  % Should it be an error to refer to X in a self assignment, or should
  % this X refer to file1's X?
  let X = X
end

%}