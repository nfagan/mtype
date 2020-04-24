%{

@T begin export

import mt.scalar

declare method single double :: [double] = (single)

declare method single >  :: [logical] = (single, single)
declare method single >= :: [logical] = (single, single)
declare method single <  :: [logical] = (single, single)
declare method single <= :: [logical] = (single, single)

declare method single subsindex :: [double] = (single)

end

%}