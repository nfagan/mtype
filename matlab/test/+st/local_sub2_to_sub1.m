%{
@T begin

import base
import st.sub2
import st.sub1
import st.base

end
%}

% @T :: [st.sub1, ?] = (st.sub2)
function [xs, o] = local_sub2_to_sub1(y)

o = sub2_method( y );
xs = st.sub1;

end

function us1 = use1

[tmp, o] = st.local_sub2_to_sub1( st.sub2 );
us1 = st.local_sub2_to_sub1( tmp );

end