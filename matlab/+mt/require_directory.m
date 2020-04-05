function p = require_directory(p)

%   REQUIRE_DIRECTORY -- Create directory if it does not already exist.
%
%     See also mt.save_path

if ( exist(p, 'dir') ~= 7 )
  mkdir( p );
end

end