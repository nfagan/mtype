%{

@T begin

import example_exports

% Because `component` is not an exported declaration of `example_exports`, 
% this re-declaration of `component` is okay.
record component
  x: double
end

end

%}

% @T :: [Color, component] = ()
function [color, component] = example_import

% @T constructor
color = struct( 'r', 1, 'g', uint8(1), 'b', uint8(2) );

% @T constructor
component = struct( 'x', 1 );

end