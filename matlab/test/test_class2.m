function test_class2()

import shared_utils.general.parsestruct;

x = shared_utils.general.parsestruct();
y = shared_utils.general.parsestruct();

% parsestruct = 1;

  function nested
    import shared_utils.general.parsestruct;
    
    z = parsestruct();
    
    function nested2
      y = parsestruct();
    end
  end

z = parsestruct;

end