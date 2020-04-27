#include "ast.hpp"
#include "def.hpp"
#include "StringVisitor.hpp"
#include "visitor.hpp"
#include "../store.hpp"
#include "../parse/identifier_classification.hpp"

namespace mt {

/*
 * RootBlock
 */

namespace {
  AstNode* extract_code_ast_node(AstNode* source) {
    if (source->is_type_annot_macro()) {
      const auto macro_ptr = static_cast<TypeAnnotMacro*>(source);
      const auto maybe_enclosed_code_node = macro_ptr->annotation->enclosed_code_ast_node();
      if (maybe_enclosed_code_node) {
        return maybe_enclosed_code_node.value();
      }
    }
    return source;
  }
}

Optional<TypeAssertion*> RootBlock::extract_top_level_type_assertion() const {
  for (const auto& node : block->nodes) {
    if (node->is_type_annot_macro()) {
      const auto macro_ptr = static_cast<const TypeAnnotMacro*>(node.get());
      if (macro_ptr->annotation->is_type_assertion()) {
        return Optional<TypeAssertion*>(static_cast<TypeAssertion*>(macro_ptr->annotation.get()));
      }
    }
  }

  return NullOpt{};
}

Optional<FunctionDefNode*> RootBlock::extract_top_level_function_def() const {
  for (const auto& node : block->nodes) {
    auto node_ptr = extract_code_ast_node(node.get());

    if (node_ptr->is_function_def_node()) {
      return Optional<FunctionDefNode*>(static_cast<FunctionDefNode*>(node_ptr));
    }
  }

  return NullOpt{};
}

Optional<int64_t> RootBlock::extract_top_level_function_def_index() const {
  const int64_t num_nodes = block->nodes.size();
  for (int64_t i = 0; i < num_nodes; i++) {
    if (block->nodes[i]->is_function_def_node()) {
      return Optional<int64_t>(i);
    }
  }

  return NullOpt{};
}

Optional<FunctionDefNode*> RootBlock::extract_constructor_function_def(const Store& store) const {
  for (const auto& node : block->nodes) {
    auto node_ptr = extract_code_ast_node(node.get());

    if (node_ptr->is_class_def_node()) {
      auto class_node = static_cast<ClassDefNode*>(node_ptr);

      for (const auto& method : class_node->method_defs) {
        const auto def_handle = method->def->def_handle;
        FunctionDefNode* maybe_ctor_node = nullptr;

        if (def_handle.is_valid()) {
          store.use<Store::ReadConst>([&](const auto& reader) {
            const auto& def = reader.at(def_handle);
            if (def.attributes.is_constructor()) {
              maybe_ctor_node = method->def.get();
            }
          });
        }

        if (maybe_ctor_node) {
          return Optional<FunctionDefNode*>(maybe_ctor_node);
        }
      }
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