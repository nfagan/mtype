function test_import(s)

x = parse_struct( struct(), {} );

import mt.parse_struct;

y = parse_struct( struct(), {} );

disp( import() );
import( s );

end

function parse_struct()
disp( 'parsing' );
end