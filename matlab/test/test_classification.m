function y = test_classification(l)

import shared_utils.general.parsestruct;

x = 1;
y = 1;
z = 1;

y = @(x, y, z) x + y + z;

  function parsestructs
%     import x.y.z;
    x = y;
    x = z.z;
    
    a = 2;
    zzzz =a;
    
    function y
      function child
%         import z.z x.y.a;
        y = z;
      end
    end
    
    x = z.z;
  end

end

function c = another(c)

% import d.c;

end