%{

@T begin export

import mt.scalar

declare function strcmp :: [logical] = (mt.str, mt.str)
declare function strncmp :: [logical] = (mt.str, mt.str)
declare function contains :: [logical] = (mt.str, mt.str)
declare function cellstr :: [mt.cellstr] = (mt.str)
declare function strjoin :: [char] = (mt.cellstr)
declare function char :: [char] = (integral)

namespace mt
  let cellstr = {list<char>}
  let str = mt.cellstr | char | string
end

end

%}  