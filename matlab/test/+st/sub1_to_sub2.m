%{
@T begin

import base
import st.sub2
import st.sub1
import st.base

end
%}

% @T :: [st.sub2, ?] = (st.sub1)
function [xs, o] = sub1_to_sub2(y)

xs = st.sub2;
o = sub1_method( y );

end