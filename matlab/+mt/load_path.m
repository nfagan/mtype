function p = load_path()

file_path = mt.path_filepath();

if ( exist(file_path, 'file') == 2 )
  p = strsplit( fileread(file_path), newline );
else
  error( 'No path file was saved in "%s".', file_path );
end

end