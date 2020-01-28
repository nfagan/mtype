function l = fixlabels(l)

import shared_utils.general.parsestruct;

  function parsestructs
    import x.y.z;
    x = y;
    x = z.z;
    
    a = 2;
    zzzz =a;
    
    function y
      function child
        import z.z x.y.a;
        y = z;
      end
    end
    
    x = z.z;
  end

end

function c = another(c)

% import d.c;

end