#include "code_file.hpp"
#include "../ast.hpp"
#include <cassert>

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
    auto node_ptr = node.get();

    if (node_ptr->represents_type_annot_macro()) {
      auto macro_ptr = static_cast<TypeAnnotMacro*>(node_ptr);
      auto maybe_enclosed_code_node = macro_ptr->annotation->enclosed_code_ast_node();
      if (maybe_enclosed_code_node) {
        node_ptr = maybe_enclosed_code_node.value();
      }
    }

    if (node_ptr->represents_class_def()) {
      return CodeFileType::class_def;

    } else if (node_ptr->represents_function_def()) {
      if (is_script) {
        return CodeFileType::script_with_local_functions;
      } else {
        return CodeFileType::function_def;
      }

    } else if (node_ptr->represents_stmt_or_stmts()) {
      is_script = true;
    }
  }

  if (is_script) {
    return CodeFileType::function_less_script;
  } else {
    return CodeFileType::unknown;
  }
}

bool CodeFileDescriptor::represents_known_file() const {
  switch (file_type) {
    case CodeFileType::none:
    case CodeFileType::unknown:
      return false;
    default:
      return true;
  }
}

std::string CodeFileDescriptor::containing_package() const {
  return fs::package_name(fs::directory_name(file_path));
}

void swap(CodeFileDescriptor& a, CodeFileDescriptor& b) {
  using std::swap;
  swap(a.file_type, b.file_type);
  swap(a.file_path, b.file_path);
}

}
