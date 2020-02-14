classdef Another
  properties
    x, y, z = 1; a = 1;
    d = 11;
  end
  
  methods    
    [a] = c();
    
    function z = Another()
      z.x = 10;
    end
    function a = b(obj, a)
      b = obj;
    end
    
    function a = test(obj)
      a = test( obj );
      
      function a = test(obj)
        a = 2;
      end
    end
  end
  
  methods
    function x = shared_utils.gui.MatrixLayout(obj)
      x = shared_utils.gui.MatrixLayout();
    end
  end
end

function c = test(x)
c = 1;
end