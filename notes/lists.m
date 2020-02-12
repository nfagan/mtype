%{



%}

% @T [double, double*] = (single, single*)
function [x, varargout] = lists(y, varargin)

end

% 1. Initially unconstrained: varargout and varargin are separate lists
% of free type variables.
function varargout = example_user1(varargin)
% 6. type(a) == first_of( output_of(example_user2) )
a = example_user2( 1, 2 );
z = { varargin{a} };

% 2. Varargout constrained by outputs of `lists` and `nargout`, determined
% by the caller.
[varargout{1:nargout}] = lists( z{:} );
end

% 3. Initially unconstrained.
function varargout = example_user2(x, y)
% 4. type(z) == {[type(x), type(y)]}
z = { x, y };
% 5. Varargout constrained by outputs of `example_user1` and `nargout`, 
% determined by the caller.
[varargout{1:nargout}] = example_user1( z{:} );
end

function varargout = match1(varargin)

persistent x;
if ( isempty(x) )
  x = 1;
end

if ( x < 100 )
  x = x + 1;
  [varargout{1:nargout}] = recursive_example( varargin{:} );
end

function varargout = recursive_example(varargin)
  [varargout{1:nargout}] = match1( varargin{:} );
end

end