classdef HetRoot < matlab.mixin.Heterogeneous
  properties
  end
  
  methods (Sealed)
    function root_disp(obj)
      disp( obj );
    end
  end
end