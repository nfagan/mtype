import shared_utils.*;

ms = {};

ms = [ ms, io.find(repget('fieldtrip'), '.m', true) ];
ms = [ ms, io.find(PsychtoolboxRoot, '.m', true) ];
ms = [ ms, io.find(repget('ptb'), '.m', true) ];

fs = eachcell( @(x) mt.to_utf8(fileread(x)), ms );

%%

tic;

status = false( size(fs) );
for i = 1:numel(fs)
  status(i) = mt.entry( fs{i}, 0, 0 );
end

toc;

%%

failed = fs(~status);
mt.entry( failed{22} );

%%

ms = shared_utils.io.find( toolboxdir(''), '.m', true );

%%

status = false( size(ms) );
n = numel( ms );

tic;

parfor i = 1:n
  f = mt.to_utf8( fileread(ms{i}) );
  status(i) = mt.entry( f, 0, 0 );
end

toc;

%%

z = find( ~status );
use_z = z(2);

mt.entry( mt.to_utf8(fileread(ms{use_z})) );