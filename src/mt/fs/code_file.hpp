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

class CodeFileDescriptor {
public:
  explicit CodeFileDescriptor(FilePath file_path) :
  file_type(CodeFileType::none), file_path(std::move(file_path)) {
    //
  }

  CodeFileDescriptor() : file_type(CodeFileType::none) {
    //
  }
  ~CodeFileDescriptor() = default;

  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(CodeFileDescriptor)
  MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(CodeFileDescriptor)

  bool represents_known_file() const;
  std::string containing_package() const;

public:
  CodeFileType file_type;
  FilePath file_path;
};

void swap(CodeFileDescriptor& a, CodeFileDescriptor& b);

}