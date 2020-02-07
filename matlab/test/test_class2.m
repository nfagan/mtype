function test_class2()

import shared_utils.general.parsestruct;

x = shared_utils.general.parsestruct();
y = shared_utils.general.parsestruct();

exist = 2;

while ( exist(file_path) ~= 0 )
  file_path = 1;
end

% parsestruct = 1;

  function nested
    import shared_utils.general.parsestruct;
    
    z = parsestruct();
    
    function nested2
      y = parsestruct();
      y = x;
    end
  end

z = parsestruct;
x = parsestruct;
x = parsestruct;

for i = 1:10
  
end

if true
  data = [];
  for i=1:Nchan
    data{i,1} = deblank(x(i,:));
  end
end

end