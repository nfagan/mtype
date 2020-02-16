%{

struct AstStore {
  BoxedRootBlock at(file_path);
  BoxedRootBlock at(fully_qualified_name);

  bool has_public_function(func_name);
  bool has_public_class(class_name);

  void emplace_root(FileInfo info, BoxedRootBlock root) {
    auto dir_path = outer_directory(info.file_path);
    CodeDirectoryHandle handle = lookup_code_directory(dir_path);
    CodeDirectoryHandle parent_handle;

    if (!handle.is_valid()) {
      handle = make_code_directory();
      auto parent_dir = outer_directory(dir_path);
      if (!parent_dir.empty()) {
        parent_handle = lookup_code_directory(parent_dir);
        if (!parent_handle.is_valid()) {
          parent_handle = make_code_directory();
        }
      }
    }

    auto& code_directory = at(handle);
    
    if (is_class_def(root)) {
      const auto class_handle = root->scope->classes[0];
      code_directory.public_classes.emplace_back(class_handle);
    } else if (is_public_function(root)) {
      const auto def_handle = root->scope->local_functions[0];
      code_directory.public_functions.emplace_back(def_handle);
    }
  }
}

struct FilePath {

};

struct CodeDirectory {
  FilePath absolute_path;
  CodeDirectoryHandle parent;
  
  vector<FunctionDefHandle> public_functions;
  vector<FunctionDefHandle> private_functions;
  vector<ClassDefHandle> public_classes;
  vector<RootBlock*> public_scripts;

  std::string package_prefix()
};

struct CodeFile {
  FileInfo file_info;
  CodeDirectoryHandle parent_directory;
}

struct FileInfo {
  bool is_anonymous() const;

  FilePath file_path;
}

classify_function(FunctionReference& ref, Types argument_types, Types return_types) {
  //
}


%}


%{

a = example2( 1, example(1, 1) );
b = example2( 0, example(1, 1) );
x = example( a, b );

function y = example(a, b)
  y = example2( a, b ) + example2( b, a );
end

function t = example2(c, d)
  % t is given type `typeof<d>` | char
  if c
    t = d;
  else
    t = '';
  end
end

%}

%{

function out = find(x, ext, rec)
  out = find_one( x, ext, rec );
end

function out = find_one(path, ext, rec)
  out = find_impl( path, ext, {} );
end

function out = find_impl(path, ext, out)
  import shared_utils.io.dirnames;

  out = [ out, dirnames(path, ext, true) ];
  folders = dirnames( path, 'folders', true );

  if ( isempty(folders) ), return; end

  for i = 1:numel(folders)
    out = find_one( folders{i}, ext, out );
  end
end

%}
