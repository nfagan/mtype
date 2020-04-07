#include "ast.hpp"
#include "def.hpp"
#include "StringVisitor.hpp"
#include "visitor.hpp"
#include "../identifier_classification.hpp"

namespace mt {

/*
 * RootBlock
 */

Optional<FunctionDefNode*> RootBlock::top_level_function_def() const {
  for (const auto& node : block->nodes) {
    auto node_ptr = node.get();

    if (node_ptr->represents_type_annot_macro()) {
      const auto macro_ptr = static_cast<TypeAnnotMacro*>(node_ptr);
      const auto maybe_enclosed_code_node = macro_ptr->annotation->enclosed_code_ast_node();
      if (maybe_enclosed_code_node) {
        node_ptr = maybe_enclosed_code_node.value();
      }
    }

    if (node_ptr->represents_function_def()) {
      return Optional<FunctionDefNode*>(static_cast<FunctionDefNode*>(node_ptr));
    }
  }

  return NullOpt{};
}

void RootBlock::accept(TypePreservingVisitor& vis) {
  vis.root_block(*this);
}

void RootBlock::accept_const(TypePreservingVisitor& vis) const {
  vis.root_block(*this);
}

RootBlock* RootBlock::accept(IdentifierClassifier& classifier) {
  return classifier.root_block(*this);
}

std::string RootBlock::accept(const StringVisitor& vis) const {
  return vis.root_block(*this);
}

/*
 * Block
 */

void Block::accept(TypePreservingVisitor& vis) {
  vis.block(*this);
}

void Block::accept_const(TypePreservingVisitor& vis) const {
  vis.block(*this);
}

std::string Block::accept(const StringVisitor& vis) const {
  return vis.block(*this);
}

Block* Block::accept(IdentifierClassifier& classifier) {
  return classifier.block(*this);
}

}