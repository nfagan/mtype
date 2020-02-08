function x = test_class2()

persistent i z;

if ( isempty(z) )
  z = 0;
end

if ( ~isempty(i) && z < 1e3 )
  z = z + 1;
  feval( i, [] );
end

x = @(x) feval(another(1, 2, 3, i, x));
i = x;

end

function z = another(varargin)

z = @() test_class2();

end