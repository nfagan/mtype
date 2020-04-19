% @T import st.base
% @T import st.sub1
% @T :: [] = ([?] = (st.base))
function use_ref1(x)

x( st.base );

end

% % @T :: [] = ([?] = (st.sub1))
function use_ref2(x)

st.use_ref1( x );

end

function user()

% Error: No such function: sub1_method(st.base) ? [T7] (20:0)
use_ref2( @(x) sub1_method(x) );

end