classdef Crash
  properties
    x;
  end
  
  methods
    function obj = Crash(x)
      obj.x = x;
    end
    
    function obj = set.x(obj, x)
      obj.x = obj;
    end
    
    function obj = test(obj)
      test( obj );
    end
  end
end