function p = save_path(paths, file_path)

%   SAVE_PATH -- Save Matlab search path to text file.
%
%     mt.save_path( {dir1, dir2} ); saves a text file 'path.txt'
%     containing new-line delimited directory paths `dir1` and `dir2`. 
%
%     The file is saved in the directory given by 
%     `fullfile(mt.project_dir(), 'data')`.
%
%     See also mt.build_entry_point

if ( nargin < 1 )
  paths = strsplit( path(), pathsep() );
end
if ( nargin < 2 )
  [file_path, data_path] = mt.path_filepath();
else
  data_path = fileparts( file_path );
end

p = strjoin( paths, newline() );

if ( ~isempty(data_path) )
  mt.require_directory( data_path );
end

fid = fopen( file_path, 'w' );
cleanup = onCleanup( @() fclose(fid) );

fprintf( fid, p );

end