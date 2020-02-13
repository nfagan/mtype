import shared_utils.*;

ms = {};

ms = [ ms, io.find(repget('fieldtrip'), '.m', true) ];
ms = [ ms, io.find(PsychtoolboxRoot, '.m', true) ];
ms = [ ms, io.find(repget('ptb'), '.m', true) ];

fs = eachcell( @(x) mt.to_utf8(fileread(x)), ms );

%%

tic;

status = false( size(fs) );
parfor i = 1:numel(fs)
  status(i) = mt.entry( fs{i}, 0, 0 );
end

toc;

%%

failed = fs(~status);
mt.entry( failed{17} );