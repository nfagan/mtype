#include "code_file.hpp"
#include "../ast.hpp"

namespace mt {

const char* to_string(CodeFileType file_type) {
  switch (file_type) {
    case CodeFileType::class_def:
      return "class_def";
    case CodeFileType::function_def:
      return "function_def";
    case CodeFileType::function_less_script:
      return "function_less_script";
    case CodeFileType::script_with_local_functions:
      return "script_with_local_functions";
    case CodeFileType::none:
      return "none";
    case CodeFileType::unknown:
      return "unknown";
  }
}

CodeFileType code_file_type_from_root_block(const RootBlock& root) {
  bool is_script = false;

  for (const auto& node : root.block->nodes) {
    if (node->represents_class_def()) {
      return CodeFileType::class_def;

    } else if (node->represents_function_def()) {
      if (is_script) {
        return CodeFileType::script_with_local_functions;
      } else {
        return CodeFileType::function_def;
      }

    } else if (node->represents_stmt_or_stmts()) {
      is_script = true;
    }
  }

  if (is_script) {
    return CodeFileType::function_less_script;
  } else {
    return CodeFileType::unknown;
  }
}

bool CodeFilePath::represents_anonymous_code() const {
  return file_path.empty();
}

}
