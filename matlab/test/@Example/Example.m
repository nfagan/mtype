classdef Example < Base
  properties
    Prop1;
  end
  methods
    function obj = Example()
    end
    
    function another(obj)
      another@Base(obj)
      disp( 'x' );
    end
    
    function obj = set.Prop1(obj, v)
%       set.Prop1@Base( v );
    end
    
    test(obj)
  end
end