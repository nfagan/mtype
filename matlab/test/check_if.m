function c = check_if(cond, a, b)

import shared_utils.general.parsestruct;

if ( cond )
  c = a;
else
  c = parsestruct( a );
end

try
  z = 2;
catch a
  z = 3;
end

e = z;

end