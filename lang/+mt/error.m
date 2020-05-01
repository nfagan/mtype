%{

@T begin

import mt.scalar

declare function error :: [] = (char, list<integral>)
declare function warning :: [] = (char, list<integral>)
declare function assert :: [] = (logical, list<integral>)

namespace matlab
  namespace detail
    record ErrorStack
      file: char
      name: char
      line: double
    end
  end

  declare classdef MException
    identifier: char
    message: char
    cause: {list<char>}
    stack: matlab.detail.ErrorStack
  end
end

end

%}