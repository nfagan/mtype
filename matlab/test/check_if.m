function c = check_if(cond, a, b)

import shared_utils.general.parsestruct;

if ( cond )
  c = a;
else
  c = parsestruct( cond );
end

end