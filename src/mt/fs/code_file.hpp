#pragma once

#include "path.hpp"
#include <vector>
#include <memory>

namespace mt {

enum class CodeFileType;
struct RootBlock;

CodeFileType code_file_type_from_root_block(const RootBlock& root);
const char* to_string(CodeFileType file_type);

enum class CodeFileType {
  class_def,
  function_def,
  function_less_script,
  script_with_local_functions,
  none,
  unknown,
};

class CodeDirectoryHandle {
  //
};

class CodeDirectory {
  FilePath absolute_path;
//  CodeDirectoryHandle parent;
};

class CodeFilePath {
public:
  CodeFilePath() = default;
  CodeFilePath(const FilePath& path) : file_path(path) {
    //
  }

  ~CodeFilePath() = default;

  bool represents_anonymous_code() const;

private:
  FilePath file_path;
};

class CodeFileDescriptor {
  CodeFileDescriptor() : file_type(CodeFileType::none) {
    //
  }
  ~CodeFileDescriptor() = default;

public:
  CodeFileType file_type;

private:
  CodeFilePath file_path;
  //  CodeDirectoryHandle parent_directory_handle;
};
}