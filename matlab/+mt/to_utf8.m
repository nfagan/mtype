function c = to_utf8(c)

%   TO_UTF8 -- Convert to utf8 encoded char vector.
%
%     See also mt.entry

c = char( unicode2native(c, 'UTF-8') );

end