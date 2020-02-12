function c = from_utf8(c)

%   FROM_UTF8 -- Convert from utf8 char vector to MATLAB char vector.
%
%     See also mt.entry, mt.to_utf8

if ( ischar(c) )
  c = real( c );
end

c = char( native2unicode(c,  'UTF-8') );

end