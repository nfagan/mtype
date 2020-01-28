function build_entry_point(varargin)

defaults = struct();
defaults.optim_level = 3;
defaults.lib_subdir = 'cmake-build-release';
defaults.lib_name = 'mt';
defaults.use_platform_lib_subdir = true;

params = mt.parse_struct( defaults, varargin );

opts = platform_compile_options();
compiler_spec = opts.compiler_spec;
addtl_c_flags = opts.addtl_c_flags;
addtl_cxx_flags = opts.addtl_cxx_flags;
addtl_comp_flags = opts.addtl_comp_flags;

this_file = which( 'mt.build_entry_point' );

mt_dir = fileparts( this_file );
matlab_dir = fileparts( mt_dir );
project_dir = fileparts( matlab_dir );

lib_subdir = get_lib_subdir( params );
lib_dir = fullfile( project_dir, lib_subdir );
lib_name = params.lib_name;

include_dir = fullfile( project_dir, 'src' );
src_dir = fullfile( matlab_dir, 'src' );

mex_func_paths = cellfun( @(x) fullfile(src_dir, x), source_files(), 'un', 0 );
mex_func_path = strjoin( mex_func_paths, ' ' );

optim_level = params.optim_level;
additional_defines = '-DNDEBUG';

build_cmd = sprintf( build_command_template_str() ...
  , compiler_spec, addtl_c_flags, addtl_cxx_flags ...
  , addtl_comp_flags ...
  , optim_level, additional_defines ...
  , optim_level, additional_defines ...
  , mex_func_path, include_dir ...
  , lib_dir, lib_name, mt_dir ...
);

eval( sprintf('mex %s', build_cmd) );

end

function files = source_files()

files = { 'entry.cpp', 'util.cpp', 'types.cpp' };

end

function s = build_command_template_str()

s = ['-v %s%s%s COMPFLAGS="$COMPFLAGS %s" COPTIMFLAGS="-O%d -fwrapv %s"' ...
  , ' CXXOPTIMFLAGS="-O%d -fwrapv %s" %s -I%s -L%s -l%s -outdir %s'];

end

function compile_options = platform_compile_options()

if ( isunix() && ~ismac() )
  % Linux
  compile_options.compiler_spec = 'GCC=''/usr/bin/gcc-4.9'' G++=''/usr/bin/g++-4.9'' ';
  compile_options.addtl_c_flags = '';
  compile_options.addtl_cxx_flags = 'CXXFLAGS="-std=c++1y -fPIC"';
  compile_options.addtl_comp_flags = '';
else
  compile_options.compiler_spec = '';
  compile_options.addtl_c_flags = '';
  
  if ( ispc() )
    compile_options.addtl_cxx_flags = '';
    compile_options.addtl_comp_flags = '/std:c++17';
  else
    compile_options.addtl_cxx_flags = 'CXXFLAGS="-std=c++14"';
    compile_options.addtl_comp_flags = '';
  end
end

end

function lib_subdir = get_lib_subdir(params)

if ( ~params.use_platform_lib_subdir )
  lib_subdir = params.lib_subdir;
else
  if ( ispc() )
    lib_subdir = 'build/Release';
  else
    lib_subdir = 'cmake-build-release';
  end
end

end