function save_path(paths)

%   SAVE_PATH -- Save Matlab search path to text file.
%
%     See also mt.build_entry_point

if ( nargin < 1 )
  paths = strsplit( path(), pathsep() );
end

p = strjoin( paths, newline() );

data_path = fullfile( fileparts(fileparts(which('mt.save_path'))), 'data' );
file_name = 'path.txt';
file_path = fullfile( data_path, file_name );

fid = fopen( file_path, 'w' );
cleanup = onCleanup( @() fclose(fid) );

fprintf( fid, p );

end