% @T :: [double] = (double, double)
function x = path_example(z, y)

x = child( y, z );

end

function z = child(a, y)

z = a * y;

end