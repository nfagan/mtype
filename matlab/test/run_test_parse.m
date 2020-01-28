import shared_utils.*;

ms = {};

ms = [ ms, io.find(repget('fieldtrip'), '.m', true) ];
ms = [ ms, io.find(PsychtoolboxRoot, '.m', true) ];

fs = eachcell( @fileread, ms );

%%

tic;

status = false( size(fs) );
for i = 1:numel(fs)
  status(i) = mt.entry( fs{i}, 0, 0 );
end

toc;

%%

failed = fs(~status);
mt.entry( failed{12} );