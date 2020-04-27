#include "external_visibility.hpp"
#include "../ast.hpp"
#include "../store.hpp"
#include "../fs/code_file.hpp"

namespace mt {

/*
 * ExtVisibilityInstance
 */

ExtVisibilityInstance::ExtVisibilityInstance(const Store& def_store) :
def_store(def_store), function_depth(0) {
  //
}

void ExtVisibilityInstance::add(const FunctionReference& ref) {
  auto& visible_functions = require_visible_functions(*ref.scope->file_descriptor);
  visible_functions[ref.name] = ref;
}

ExtVisibleFunctions&
ExtVisibilityInstance::require_visible_functions(const CodeFileDescriptor& for_file) {
  if (visible_functions_by_file.count(for_file.file_path) == 0) {
    visible_functions_by_file[for_file.file_path] = {};
  }
  return visible_functions_by_file.at(for_file.file_path);
}

/*
 * ExtVisibility
 */

ExtVisibility::ExtVisibility(ExtVisibilityInstance* instance) :
instance(instance) {
  //
}

void ExtVisibility::root_block(const RootBlock& root) {
  root.block->accept_const(*this);
}

void ExtVisibility::block(const Block& block) {
  for (const auto& node : block.nodes) {
    node->accept_const(*this);
  }
}

/*
 * TypeAnnot
 */

void ExtVisibility::type_annot_macro(const TypeAnnotMacro& macro) {
  macro.annotation->accept_const(*this);
}

void ExtVisibility::type_assertion(const TypeAssertion& node) {
  node.node->accept_const(*this);
}

/*
 * Def
 */

void ExtVisibility::function_def_node(const FunctionDefNode& node) {
  if (instance->function_depth == 0) {
    instance->add(instance->def_store.get(node.ref_handle));
  }
}

void ExtVisibility::class_def_node(const ClassDefNode& node) {
  for (const auto& method : node.method_defs) {
    const auto& ref_handle = method->def->ref_handle;
    const auto ref = instance->def_store.get(ref_handle);
    const auto attrs = instance->def_store.get_attributes(ref.def_handle);

    if (attrs.is_public() && (attrs.is_constructor() || attrs.is_static())) {
      instance->add(ref);
    }
  }
}

}
