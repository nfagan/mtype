classdef SubsrefTest
  methods (Static = true)
    function z = example()
      %
      %
      a = {1, 1}; % tuple list double, size = (1, 2)
      b = {'a', 'a'};
      
      [a{:}] = deal( a{:} );  % rhs a{:} -> destructured tuple, num outputs = result
      
      % An l-value destructured tuple of size S can match an r-value 
      % destructured tuple of size S' >= S, matching arguments 1:S
      % The size of an l-value destructured tuple must be known at
      % compile-time.
      %
      % a = {1, 1}; % maybe size (1, 2)
      % [a{:}] = deal( a{:} ); % must be size (1, 2)
      %
      % Suposition -> Requirement
      
      % l-value destructured tuple [a, b]; size: constexpr 2
      %   = 
      % r-value destructured tuple (function outputs; size of l-value)
%       [a, b] = another( 1, 2 );

      % l-value destructured tuple [a, b]; size: constexpr 2
      %   =
      % r-value destructured tuple (function outputs): result of z{:})
      z = { 1, 2, 3 };
      [d, e] = z{:};
      
      % l-value destructured tuple [a{:}, b{:}] containing destructured
      % tuples a{:}, b{:}; a{:} matches inputs 1:size(a); b{:} matches
      % inputs size(a)+1:end;
      % num outputs (another) given by size [a{:}, b{:}].
      [a{:}, b{:}] = another( 1, 2, 3, func() );
      
      % out = another(destructured_tuple(1, 2, 3, func()))
      % destructured tuple as function output matches 1 arg as function
      % input.
      
      % out = another(destructured_tuple(1, a{:}, func()))
      % if (rhs)
      %   num outputs (func()) = constexpr 1
      % else
      %   num outputs = num outputs(destructured_tuple(lhs))
      [a{:}, b{:}] = another(1, a{:}, func());
      
      % lhs -> destructured tuple of destructured tuple.
      [c,c,c,c] = deal( a{:}, b{:} );
      
      [a{:}, b{:}] = deal( a{:}, b{:} );
      
      z = func2( {1}, 1 );
      
      function [f, a] = func()
        f = 1;
        a = 2;
      end
      
      function f = func2(a, b)
        % a: tuple(list(t0)); size: unknown
        % 
        [a{b}] = 2;
        f = a;
      end
      
      function varargout = another(varargin)
        [varargout{1:nargout}] = varargin{:};
      end
    end
    
    function another()
      
    end
  end
end