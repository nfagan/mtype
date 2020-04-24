%{

@T begin export

import mt.scalar

declare method int64 .* :: [int64] = (int64, int64)
declare method int64 ./ :: [int64] = (int64, int64)
declare method int64 - :: [int64] = (int64, int64)
declare method int64 + :: [int64] = (int64, int64)

declare method int64 subsindex :: [double] = (int64)

end

%}