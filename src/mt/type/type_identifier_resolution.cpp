#include "type_identifier_resolution.hpp"
#include "type_scope.hpp"
#include "../ast.hpp"
#include "../store.hpp"
#include "type_store.hpp"
#include <cassert>

namespace mt {

/*
 * TypeIdentifierResolverInstance::UnresolvedIdentifier
 */

TypeIdentifierResolverInstance::UnresolvedIdentifier::UnresolvedIdentifier(const TypeIdentifier& ident,
                                                                           TypeScope* scope) :
                                                                           identifier(ident), scope(scope) {
  //
}

/*
 * TypeIdentifierResolverInstance
 */

TypeIdentifierResolverInstance::TypeIdentifierResolverInstance(TypeStore& type_store, const Store& def_store) :
type_store(type_store), def_store(def_store) {
  //
}

void TypeIdentifierResolverInstance::add_error(BoxedTypeError err) {
  errors.push_back(std::move(err));
}

bool TypeIdentifierResolverInstance::had_error() const {
  return !errors.empty();
}

void TypeIdentifierResolverInstance::add_unresolved_identifier(const TypeIdentifier& ident, TypeScope* in_scope) {
  unresolved_identifiers.emplace_back(ident, in_scope);
}

/*
 * TypeIdentifierResolver
 */

TypeIdentifierResolver::TypeIdentifierResolver(TypeIdentifierResolverInstance* instance) :
instance(instance) {
  //
}

/*
 * Block
 */

void TypeIdentifierResolver::root_block(const RootBlock& block) {
  ScopeState<TypeScope>::Helper scope_helper(instance->scopes, block.type_scope);
  block.block->accept_const(*this);
}

void TypeIdentifierResolver::block(const Block& block) {
  for (const auto& node : block.nodes) {
    node->accept_const(*this);
  }
}

/*
 * TypeAnnot
 */

void TypeIdentifierResolver::type_annot_macro(const TypeAnnotMacro& macro) {
  macro.annotation->accept_const(*this);
}

void TypeIdentifierResolver::inline_type(const InlineType& node) {
  node.type->accept_const(*this);
}

void TypeIdentifierResolver::type_begin(const TypeBegin& begin) {
  TypeIdentifierExportState::Helper state_helper(instance->export_state, begin.is_exported);

  for (const auto& node : begin.contents) {
    node->accept_const(*this);
  }
}

void TypeIdentifierResolver::type_let(const TypeLet& node) {
  node.equal_to_type->accept_const(*this);
}

void TypeIdentifierResolver::function_type_node(const FunctionTypeNode& node) {
  for (const auto& arg : node.inputs) {
    arg->accept_const(*this);
  }
  for (const auto& out : node.outputs) {
    out->accept_const(*this);
  }
}

void TypeIdentifierResolver::scalar_type_node(const ScalarTypeNode& node) {
  const auto& ident = node.identifier;
  auto* scope = instance->scopes.current();

  const auto maybe_ref = scope->lookup_type(node.identifier);
  if (!maybe_ref) {
    instance->add_unresolved_identifier(ident, scope);
  }
}

void TypeIdentifierResolver::record_type_node(const RecordTypeNode& node) {
  for (const auto& field : node.fields) {
    field.type->accept_const(*this);
  }
}

/*
 * Def
 */

void TypeIdentifierResolver::function_def_node(const FunctionDefNode& node) {
  ScopeState<TypeScope>::Helper scope_helper(instance->scopes, node.type_scope);
  Block* body = nullptr;

  instance->def_store.use<Store::ReadConst>([&](const auto& reader) {
    body = reader.at(node.def_handle).body.get();
  });

  if (body) {
    body->accept_const(*this);
  }
}

void TypeIdentifierResolver::class_def_node(const ClassDefNode& node) {
  for (const auto& method : node.method_defs) {
    method->accept_const(*this);
  }
}

}
