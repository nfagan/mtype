function [b, nc] = terminally_recursive(a)

% 1. a: `t0`

persistent num_calls;

if ( nargin == 0 )
  % 2. a: `double | char`; scalar
  if ( rand() < 0.5 )
    a = 1;
  else
    a = 'x';
  end
  
  num_calls = 0;
end

b = 1;
num_calls = num_calls + 1;

while ( a < 5.1e5 )
  [a, b] = local( a );
end

nc = num_calls;

end

function [a, b] = local(varargin)

% 3. varargin: `ts...`
z = randi( max(varargin{:}, 1e6) );

% 4. 
[b, nc] = terminally_recursive( z );

% 5. a: `typeof z`
a = z;

% 6. b: `typeof horzcat ts`
b = [varargin{:}];

end