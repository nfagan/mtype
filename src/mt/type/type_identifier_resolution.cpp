#include "type_identifier_resolution.hpp"
#include "type_scope.hpp"
#include "../ast.hpp"
#include "../store.hpp"
#include "library.hpp"
#include "type_store.hpp"
#include <cassert>

namespace mt {

/*
 * TypeIdentifierResolverInstance::UnresolvedIdentifier
 */

void TypeIdentifierResolverInstance::TypeCollectorState::Stack::push(TypeCollectorState& collector) {
  collector.push();
}

void TypeIdentifierResolverInstance::TypeCollectorState::Stack::pop(TypeCollectorState& collector) {
  collector.pop();
}

void TypeIdentifierResolverInstance::TypeCollectorState::push() {
  type_collectors.emplace_back();
}

void TypeIdentifierResolverInstance::TypeCollectorState::pop() {
  assert(!type_collectors.empty());
  type_collectors.pop_back();
}

TypeIdentifierResolverInstance::TypeCollector& TypeIdentifierResolverInstance::TypeCollectorState::current() {
  assert(!type_collectors.empty());
  return type_collectors.back();
}

TypeIdentifierResolverInstance::TypeCollector::TypeCollector() : had_error(false) {
  //
}

void TypeIdentifierResolverInstance::TypeCollector::push(Type* type) {
  types.push_back(type);
}

void TypeIdentifierResolverInstance::TypeCollector::mark_error() {
  had_error = true;
}

TypeIdentifierResolverInstance::UnresolvedIdentifier::UnresolvedIdentifier(const TypeIdentifier& ident,
                                                                           TypeScope* scope) :
                                                                           identifier(ident), scope(scope) {
  //
}

/*
 * TypeIdentifierResolverInstance
 */

TypeIdentifierResolverInstance::TypeIdentifierResolverInstance(TypeStore& type_store, Library& library, const Store& def_store) :
type_store(type_store), library(library), def_store(def_store) {
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
  TypeIdentifierResolverInstance::TypeCollectorState::Helper collect_helper(instance->collectors);
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

namespace {
  void visit_array(TypeIdentifierResolver& resolver, const std::vector<BoxedType>& types) {
    for (const auto& type : types) {
      type->accept_const(resolver);
    }
  }
  Optional<TypePtrs> collect_types(TypeIdentifierResolver& resolver,
                                   TypeIdentifierResolverInstance& instance,
                                   const std::vector<BoxedType>& types) {
    TypeIdentifierResolverInstance::TypeCollectorState::Helper helper(instance.collectors);
    visit_array(resolver, types);
    auto& collector = instance.collectors.current();
    if (collector.had_error) {
      return NullOpt{};
    } else {
      return Optional<TypePtrs>(std::move(collector.types));
    }
  }
}

void TypeIdentifierResolver::function_type_node(const FunctionTypeNode& node) {
  auto maybe_inputs = collect_types(*this, *instance, node.inputs);
  if (!maybe_inputs) {
    instance->collectors.current().mark_error();
    return;
  }
  auto maybe_outputs = collect_types(*this, *instance, node.outputs);
  if (!maybe_outputs) {
    instance->collectors.current().mark_error();
    return;
  }
  auto inputs = instance->type_store.make_input_destructured_tuple(std::move(maybe_inputs.rvalue()));
  auto outputs = instance->type_store.make_output_destructured_tuple(std::move(maybe_outputs.rvalue()));
  auto func = instance->type_store.make_abstraction(inputs, outputs);
  instance->collectors.current().push(func);
}

void TypeIdentifierResolver::scalar_type_node(const ScalarTypeNode& node) {
  const auto& ident = node.identifier;
  auto* scope = instance->scopes.current();

  const auto maybe_ref = scope->lookup_type(node.identifier);
  if (!maybe_ref) {
    instance->add_unresolved_identifier(ident, scope);
    instance->collectors.current().mark_error();
  } else {
    instance->collectors.current().push(maybe_ref.value()->type);
  }
}

void TypeIdentifierResolver::record_type_node(const RecordTypeNode& node) {
  for (const auto& field : node.fields) {
    TypeIdentifierResolverInstance::TypeCollectorState::Helper collector_helper(instance->collectors);
    field.type->accept_const(*this);

    auto& collector = instance->collectors.current();
    if (collector.had_error) {
      return;
    }

    assert(collector.types.size() == 1);
    auto field_name = instance->type_store.make_constant_value(field.name);
    auto field_type = collector.types[0];

    node.type->fields.push_back(types::Record::Field{field_name, field_type});
  }
}

void TypeIdentifierResolver::declare_type_node(const DeclareTypeNode& node) {
  switch (node.kind) {
    case DeclareTypeNode::Kind::scalar:
      scalar_type_declaration(node);
      break;
    case DeclareTypeNode::Kind::method:
      method_type_declaration(node);
      break;
  }
}

void TypeIdentifierResolver::scalar_type_declaration(const DeclareTypeNode& node) {
  //  @TODO: Make type in ast_gen.cpp
  auto type = instance->library.make_named_scalar_type(node.identifier);
  auto maybe_scalar = instance->scopes.current()->lookup_type(node.identifier);
  assert(maybe_scalar && !maybe_scalar.value()->type);
  maybe_scalar.value()->type = type;
}

void TypeIdentifierResolver::method_type_declaration(const DeclareTypeNode& node) {
  auto& library = instance->library;
  auto& method_store = library.method_store;

  auto maybe_class = library.lookup_class(node.identifier);
  if (!maybe_class) {
    instance->add_unresolved_identifier(node.identifier, instance->scopes.current());
    return;
  }

  TypeIdentifierResolverInstance::TypeCollectorState::Helper collector_helper(instance->collectors);
  node.maybe_method.type->accept_const(*this);
  auto& current_collector = instance->collectors.current();
  if (current_collector.had_error) {
    return;
  }

  assert(current_collector.types.size() == 1 && current_collector.types[0]->is_abstraction());
  auto& method = MT_ABSTR_MUT_REF(*current_collector.types[0]);

  switch (node.maybe_method.kind) {
    case DeclareTypeNode::Method::Kind::unary_operator:
      method.assign_kind(node.maybe_method.unary_operator);
      break;
    case DeclareTypeNode::Method::Kind::binary_operator:
      method.assign_kind(node.maybe_method.binary_operator);
      break;
    case DeclareTypeNode::Method::Kind::function:
      method.assign_kind(to_matlab_identifier(node.maybe_method.name));
      break;
  }

  if (method_store.has_method(maybe_class.value(), method)) {
    std::cout << "ERROR: Duplicate method." << std::endl;
    return;
  }

  method_store.add_method(maybe_class.value(), method, &method);
}

void TypeIdentifierResolver::namespace_type_node(const NamespaceTypeNode& node) {
  TypeIdentifierNamespaceState::Helper namespace_helper(instance->namespace_state, node.identifier);
  for (const auto& enclosed : node.enclosing) {
    enclosed->accept_const(*this);
  }
}

/*
 * Def
 */

void TypeIdentifierResolver::function_def_node(const FunctionDefNode& node) {
  ScopeState<TypeScope>::Helper scope_helper(instance->scopes, node.type_scope);
  Block* body = instance->def_store.get_block(node.def_handle);

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
