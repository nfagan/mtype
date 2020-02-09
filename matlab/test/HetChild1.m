classdef HetChild1 < HetRoot
  methods
  end
  
  methods (Sealed)
    function test1(obj)
      disp( obj );
    end
  end
end