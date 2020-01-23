classdef Base
  properties
    Prop1;
  end
  
  methods
    function obj = Base()
    end
    function another(obj)
      disp( 'y' );
    end
    function tests(obj)
    end
    function obj = set.Prop1(obj, v)
      obj.Prop1 = v;
    end
  end
end