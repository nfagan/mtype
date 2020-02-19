classdef (Sealed) Crash < Another
  properties
    x;
  end
  
  properties (SetAccess = immutable)
    zz = 10;
  end
  
  methods
    function obj = Crash(x)
      %{     
      @T begin
        let X = Y
      end
      %}
      obj@Another( Another() );
      obj.x = x;
      Another();
      
      function z = Another()
        z = 1;
      end
      
      obj.zz = 2;
    end
    
    function obj = set.x(obj, x)
      obj.x = obj;
    end
    
    function obj = test(obj)
      test( obj );
    end
  end
end