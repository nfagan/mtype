%{

@T begin

import mt.scalar
import mt.string

declare function nan :: [double] = (list<numeric>)
declare function zeros :: [double] = (list<numeric>)
declare function ones :: [double] = (list<numeric>)
declare function inf :: [double] = (list<numeric>)
declare function true :: [logical] = (list<numeric>)
declare function false :: [logical] = (list<numeric>)

declare function isnan :: [logical] = (numeric)
declare function isinf :: [logical] = (double | single)

declare function nargin :: [double] = ()
declare function nargout :: [double] = ()
declare function narginchk :: [] = (double, double)
declare function nargoutchk :: [] = (double, double)

declare function mfilename :: [char] = ()
declare function class :: given T [char] = (T)

declare function isa :: given T [logical] = (T, char)
declare function numel :: given T [double] = (T)
declare function size :: given T [list<double>] = (T)
declare function length :: given T [double] = (T)
declare function isempty :: given T [logical] = (T)
declare function isequal :: given T [logical] = (T, T)

declare function error :: [] = (list<char>)

declare function tic :: [uint64] = ()
declare function toc :: [double] = (list<uint64>)

declare function repmat :: given T [T] = (T, list<numeric>)

declare function sum :: [double] = (numeric)

end

%}