function test_call_script()
% @T script test_script
test_script;
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