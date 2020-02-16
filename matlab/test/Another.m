classdef Another
  properties (Constant = true)
    Z = test( 1 );
  end
  
  properties
    X, y, z = 1; a = 1;
    d = test( 1 );
    e = Another.Z;
  end
  
  methods    
    [a] = c();
    
    function obj = set.X(obj, a)
      obj.X = obj;
    end
    
    function z = Another(x)
      z.X = x;
    end
    
    function a = b(obj, a)
      b = obj;
    end
    
    function x = horzcat(varargin)
      %
    end
    
    function a = test(obj)
      a = test( obj );
      
%       function a = test(obj)
%         a = 2;
%       end
    end
  end
  
  methods
    function y = fcat(obj)
      y = 1;
    end
    
    function x = shared_utils.gui.MatrixLayout(obj)
      x = shared_utils.gui.MatrixLayout();
    end
  end
end

function c = test(x)

c = 1;

end