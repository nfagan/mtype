%{

@T begin

declare classdef inputParser
  FunctionName: string
  CaseSensitive: logical
  KeepUnmatched: logical
  PartialMatching: logical
  StructExpand: logical
end

declare function inputParser :: [inputParser] = ()

declare method inputParser addRequired :: given T [] = (inputParser, char, [logical] = (T))
declare method inputParser addOptional :: given T [T] = (inputParser, char, T, [logical] = (T))
declare method inputParser addParameter :: given T [] = (inputParser, char, T, list<[logical] = (T)>)

end

%}