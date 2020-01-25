function test_call_script(a)

if ( a )
  % @T include
  test_script;
end

disp( x );

a();
end

function y = a()
  disp( 'in parent' );

  d = a();

  function y = a()
    disp( 'in child' );
    y = 1;
  end
end