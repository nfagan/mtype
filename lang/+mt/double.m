%{

@T begin export

import mt.scalar

declare method double int8   :: [int8] = (double)
declare method double int16  :: [int16] = (double)
declare method double int32  :: [int32] = (double)
declare method double int64  :: [int64] = (double)
declare method double uint8  :: [uint8] = (double)
declare method double uint16 :: [uint16] = (double)
declare method double uint32 :: [uint32] = (double)
declare method double uint64 :: [uint64] = (double)
declare method double single :: [single] = (double)

declare method double .' :: [double] = (double)
declare method double +  :: [double] = (double, double)
declare method double -  :: [double] = (double, double)
declare method double *  :: [double] = (double, double)
declare method double /  :: [double] = (double, double)
declare method double \  :: [double] = (double, double)
declare method double .* :: [double] = (double, double)
declare method double ./ :: [double] = (double, double)
declare method double .\ :: [double] = (double, double)
declare method double :  :: [double] = (double, double)

declare method double +  :: [double] = (double)
declare method double -  :: [double] = (double)

declare method double &  :: [logical] = (double, double)
declare method double |  :: [logical] = (double, double)
declare method double >  :: [logical] = (double, double)
declare method double >= :: [logical] = (double, double)
declare method double <  :: [logical] = (double, double)
declare method double <= :: [logical] = (double, double)
declare method double == :: [logical] = (double, double)
declare method double ~= :: [logical] = (double, double)

declare method double subsindex :: [double] = (double)

declare method double abs :: [double] = (double)
declare method double any :: [logical] = (double)
declare method double all :: [logical] = (double)

end

%}