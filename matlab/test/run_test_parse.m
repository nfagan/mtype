import shared_utils.*;

ms = {};

ms = [ ms, io.find(repget('fieldtrip'), '.m', true) ];
ms = [ ms, io.find(PsychtoolboxRoot, '.m', true) ];

fs = eachcell( @(x) char(unicode2native(fileread(x), 'UTF-8')), ms );

%%

tic;

status = false( size(fs) );
parfor i = 1:numel(fs)
  status(i) = mt.entry( fs{i}, 0, 0 );
end

toc;

%%

failed = fs(~status);
mt.entry( failed{15} );