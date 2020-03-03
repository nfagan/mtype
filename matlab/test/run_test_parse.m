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
fs = cell( size(ms) );
parfor i = 1:numel(ms)
  fs{i} = mt.to_utf8( fileread(ms{i}) );
end

%%

status = false( size(fs) );
n = numel( fs );

tic;

parfor i = 1:n
  status(i) = mt.entry( fs{i}, 0, 0 );
end

toc;

%%

z = find( ~status );
use_z = z(2);

mt.entry( mt.to_utf8(fileread(ms{use_z})) );