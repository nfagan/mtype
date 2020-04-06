% @T :: [double] = (double)
function x = path_example(z)

import shared_utils.general.parsestruct;

x = child( z );

end

function z = child(a)

z = a * a;

end