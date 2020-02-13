classdef Another
  properties
    x, y, z = 1; a = 1;
  end
  
  methods (Access = private)
  end
  
  methods
    function obj = set.x(obj, x)
    end
    function x = get.x(obj)
    end
  end
end