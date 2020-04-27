#pragma once

#include "../ast/visitor.hpp"
#include "../identifier.hpp"
#include "../definitions.hpp"
#include "../fs/path.hpp"

namespace mt {

class Store;

using ExtVisibleFunctions = std::unordered_map<MatlabIdentifier,
                                               FunctionReference,
                                               MatlabIdentifier::Hash>;

using ExtVisibleFunctionsByFile = std::unordered_map<FilePath,
                                                     ExtVisibleFunctions,
                                                     FilePath::Hash>;

struct ExtVisibilityInstance {
  ExtVisibilityInstance(const Store& def_store);

  void add(const FunctionReference& ref);
  ExtVisibleFunctions& require_visible_functions(const CodeFileDescriptor& for_file);

  const Store& def_store;
  ExtVisibleFunctionsByFile visible_functions_by_file;
  int function_depth;
};

class ExtVisibility : public TypePreservingVisitor {
  ExtVisibility(ExtVisibilityInstance* instance);

  void root_block(const RootBlock& root) override;
  void block(const Block& block) override;

  void type_annot_macro(const TypeAnnotMacro& macro) override;
  void type_assertion(const TypeAssertion& node) override;

  void function_def_node(const FunctionDefNode& node) override;
  void class_def_node(const ClassDefNode& node) override;

private:
  ExtVisibilityInstance* instance;
};

}