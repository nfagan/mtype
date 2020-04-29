% @T import mt.base
function c = use_polymorphic_add()

c = polymorphic_add( 1, 2 );
d = polymorphic_add( int64(1), int64(1) );
e = polymorphic_add( single(1), single(2) );
f = polymorphic_add( "", "" );  % error.

end