classdef TestObj
  properties
    X = 1;
  end
  
  methods
    function obj = TestObj(x)
      if ( nargin > 0 )
        validateattributes( x, {'double'}, {'scalar'}, mfilename, 'x' );
        obj.X = x;
      end
    end
    
    function obj = plus(obj, b)
      scalar = numel( b ) == 1;
      
      for i = 1:numel(obj)
        if ( scalar )
          obj(i).X = obj(i).X + b.X;
        else
          obj(i).X = obj(i).X + b(i).X;
        end
      end
    end
    
    function obj = minus(obj, b)
      obj = arrayfun( @(a, b) TestObj(a.X - b.X), obj, b );
    end
  end
end