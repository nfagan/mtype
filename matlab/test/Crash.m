classdef (Sealed) Crash < Another
  properties
    x;
  end
  
  methods
    function obj = Crash(x)
      obj@Another( Another() );
      obj.x = x;
      Another();
      
      function z = Another()
        z = 1;
      end
    end
    
    function obj = set.x(obj, x)
      obj.x = obj;
    end
    
    function obj = test(obj)
      test( obj );
    end
  end
end