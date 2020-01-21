pt_root = PsychtoolboxRoot();
chron_root = fullfile( repdir(), 'chronux' );
ft_root = fullfile( repdir(), 'fieldtrip' );

% rep_roots = { ...
%     repget('shared_utils') ...
%   , repget('dsp3'), repget('bfw'), repget('dsp2'), repget('ptb') ...
%   , repget('jja') ...
%   , repget('fieldtrip'), pt_root ...
%   , 
% };
rep_roots = { repdir };

% src_dirs = { chron_root, pt_root, ft_root };
src_dirs = rep_roots;

ms = shared_utils.io.find( src_dirs, '.m', true );

state = false( size(ms) );
ref_state = false( size(state) );

tic;
for i = 1:numel(ms)
  f = fileread( ms{i} );
  state(i) = mt.entry( f );
%   ref_state(i) = ~any( strcmp(kinds(mtree(f)), 'ERR') );
end
toc;

failed = ms(~state);

%%

f = fileread( failed{1} );
mt.entry( f );

% if(isoctavemesh) randseed=randseed+3; end

%%

f = fileread( which('dsp2.analysis.pac.multitrial_pac') );
mt.entry( f );