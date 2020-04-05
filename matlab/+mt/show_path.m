function show_path()

try
  p = mt.load_path();
  fprintf( '\n' );
  disp( p(:) );
catch
end

end