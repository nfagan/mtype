function c = check_if(cond, a, b)

if ( cond )
  c = a;
else
  c = b;
end

switch ( a )
  case true
    c = strcmp( a, 'c' );
  case false
    c = b;
end

end