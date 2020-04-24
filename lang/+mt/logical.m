%{

@T begin export

import mt.scalar

declare method logical & :: [logical] = (logical, logical)
declare method logical | :: [logical] = (logical, logical)
declare method logical && :: [logical] = (logical, logical)
declare method logical || :: [logical] = (logical, logical)

declare method logical ~ :: [logical] = (logical)

declare method logical and :: [logical] = (logical, logical)
declare method logical or  :: [logical] = (logical, logical)
declare method logical xor :: [logical] = (logical, logical)

declare method logical any :: [logical] = (logical)
declare method logical all :: [logical] = (logical)
declare method logical not :: [logical] = (logical)

end

%}