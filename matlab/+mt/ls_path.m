function ls_path()

file_path = mt.path_filepath();

if ( exist(file_path, 'file') == 2 )
  p = fileread( file_path );
end

end