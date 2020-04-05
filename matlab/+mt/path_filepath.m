function [file_path, data_path] = path_filepath()

data_path = fullfile( mt.project_dir(), 'data' );
file_name = 'path.txt';
file_path = fullfile( data_path, file_name );

end