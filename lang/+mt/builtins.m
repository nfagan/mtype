%{

@T begin

import mt.scalar
import mt.string
import mt.float

declare function nan :: [double] = (list<numeric>)
declare function NaN :: [double] = (list<numeric>)
declare function zeros :: [double] = (list<numeric>)
declare function ones :: [double] = (list<numeric>)
declare function inf :: [double] = (list<numeric>)
declare function Inf :: [double] = (list<numeric>)

declare function true :: [logical] = (list<numeric>)
declare function false :: [logical] = (list<numeric>)

declare function isa :: given T [logical] = (T, char)
declare function isstruct :: given T [logical] = (T)
declare function iscell :: given T [logical] = (T)
declare function isnan :: [logical] = (numeric)
declare function isinf :: [logical] = (mt.float)

declare function nargin :: [double] = ()
declare function nargout :: [double] = ()
declare function narginchk :: [] = (double, double)
declare function nargoutchk :: [] = (double, double)

declare function mfilename :: [char] = ()
declare function class :: given T [char] = (T)

declare function numel :: given T [double] = (T)
declare function size :: given T [list<double>] = (T)
declare function length :: given T [double] = (T)
declare function isempty :: given T [logical] = (T)
declare function isequal :: given T [logical] = (T, T)

declare function tic :: [uint64] = ()
declare function toc :: [double] = (list<uint64>)

declare function repmat :: given T [T] = (T, list<numeric>)

declare function sum :: [double] = (numeric)
declare function nnz :: [double] = (numeric | logical)

end

%}