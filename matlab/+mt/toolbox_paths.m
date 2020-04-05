function ps = toolbox_paths()

p = strsplit( path, pathsep );
boxdir = toolboxdir( '' );
keep_tb = false( size(p) );

for i = 1:numel(p)
  keep_tb(i) = startsWith( p{i}, boxdir );
end

ps = p(keep_tb);

end