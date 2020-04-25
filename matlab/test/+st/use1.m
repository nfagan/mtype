% @T import st.sub1
% @T :: [st.sub1, ?] = ()
function [us1, us2] = use1

[tmp, o] = st.sub2_to_sub1( st.sub2 );

% Should be ok: tmp is a st.sub1
us1 = st.sub1_to_sub2( tmp );

% Error: st.sub2 /= st.sub1
us2 = st.sub2_to_sub1( tmp );

end