#include "type_identifier_resolution.hpp"
#include "type_scope.hpp"
#include "../ast.hpp"
#include "../store.hpp"
#include "../string.hpp"
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

TypeIdentifierResolverInstance::TypeCollector& TypeIdentifierResolverInstance::TypeCollectorState::parent() {
  assert(type_collectors.size() > 1);
  return type_collectors[type_collectors.size()-2];
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

TypeIdentifierResolverInstance::TypeIdentifierResolverInstance(TypeStore& type_store,
                                                               Library& library,
                                                               const Store& def_store,
                                                               const StringRegistry& string_registry,
                                                               const ParseSourceData& source_data) :
type_store(type_store), library(library), def_store(def_store), string_registry(string_registry),
source_data(source_data) {
  //
}

void TypeIdentifierResolverInstance::add_error(const ParseError& err) {
  errors.push_back(err);
}

void TypeIdentifierResolverInstance::mark_parent_collector_error() {
  collectors.parent().mark_error();
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
  //  Push root type scope.
  ScopeState<TypeScope>::Helper scope_helper(instance->scopes, block.type_scope);
  ScopeState<const MatlabScope>::Helper matlab_scope_helper(instance->matlab_scopes, block.scope);
  //  Push empty type collector.
  TypeIdentifierResolverInstance::TypeCollectorState::Helper collect_helper(instance->collectors);
  //  Push monomorphic functions.
  BooleanState::Helper polymorphic_helper(instance->polymorphic_function_state, false);

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

void TypeIdentifierResolver::type_assertion(const TypeAssertion& node) {
#if MT_ALTERNATE_TYPE_ASSSERT
  alternate_type_assertion(node);
#else
  node.has_type->accept_const(*this);
  node.node->accept_const(*this);
#endif
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

  ParseError make_error_unresolved_type_identifier(const TypeIdentifierResolverInstance& instance,
                                                   const Token& token, const std::string& ident) {
    auto msg = "Unresolved type identifier: `" + ident + "`.";
    return ParseError(instance.source_data.source, token, msg, instance.source_data.file_descriptor);
  }

  ParseError make_error_duplicate_method(const TypeIdentifierResolverInstance& instance,
                                         const Token& token) {
    const auto msg = "Duplicate method.";
    return ParseError(instance.source_data.source, token, msg, instance.source_data.file_descriptor);
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
    const auto str_ident = instance->string_registry.at(node.identifier.full_name());
    instance->add_error(make_error_unresolved_type_identifier(*instance, node.source_token, str_ident));
    instance->add_unresolved_identifier(ident, scope);
    instance->collectors.current().mark_error();
  } else {
    instance->collectors.current().push(maybe_ref.value()->type);
  }
}

void TypeIdentifierResolver::infer_type_node(const InferTypeNode& node) {
  assert(node.type);
  instance->collectors.current().push(node.type);
}

void TypeIdentifierResolver::fun_type_node(const FunTypeNode& node) {
  //  Push polymorphic functions
  BooleanState::Helper polymorphic_helper(instance->polymorphic_function_state, true);
  node.definition->accept_const(*this);
}

void TypeIdentifierResolver::record_type_node(const RecordTypeNode& node) {
  for (const auto& field : node.fields) {
    TypeIdentifierResolverInstance::TypeCollectorState::Helper collector_helper(instance->collectors);
    field.type->accept_const(*this);

    auto& collector = instance->collectors.current();
    if (collector.had_error) {
      instance->mark_parent_collector_error();
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
  auto maybe_scalar = instance->scopes.current()->lookup_type(node.identifier);
  assert(maybe_scalar && maybe_scalar.value()->type);
  if (maybe_scalar) {
    //
  }
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
    instance->mark_parent_collector_error();
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
    instance->add_error(make_error_duplicate_method(*instance, node.source_token));
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
  ScopeState<const MatlabScope>::Helper matlab_scope_helper(instance->matlab_scopes, node.scope);
  ScopeState<TypeScope>::Helper scope_helper(instance->scopes, node.type_scope);

  MatlabIdentifier function_name;
  const Block* body = nullptr;
  FunctionAttributes attributes;

  TypePtrs inputs;
  TypePtrs outputs;
  TypePtrs all_parameters;

  const auto& matlab_scope = *instance->matlab_scopes.current();

  instance->def_store.use<Store::ReadConst>([&](const auto& reader) {
    const auto& def = reader.at(node.def_handle);
    function_name = def.header.name;

    for (const auto& arg : def.header.inputs) {
      if (!arg.is_ignored) {
        auto input = instance->library.require_local_variable_type(matlab_scope.local_variables.at(arg.name));
        inputs.push_back(input);
        all_parameters.push_back(input);
      }
    }
    for (const auto& out : def.header.outputs) {
      auto output = instance->library.require_local_variable_type(matlab_scope.local_variables.at(out));
      outputs.push_back(output);
      all_parameters.push_back(output);
    }

    body = def.body.get();
    attributes = def.attributes;
  });

  auto& store = instance->type_store;
  const auto input = store.make_input_destructured_tuple(std::move(inputs));
  const auto output = store.make_output_destructured_tuple(std::move(outputs));
  auto abstr = store.make_abstraction(function_name, node.ref_handle, input, output);

  Type* emplaced_type = abstr;
  if (instance->polymorphic_function_state.value()) {
    emplaced_type = store.make_scheme(abstr, std::move(all_parameters));
  }

  instance->library.emplace_local_function_type(node.def_handle, emplaced_type);

  if (body) {
    //  Push monomorphic functions.
    BooleanState::Helper polymorphic_helper(instance->polymorphic_function_state, false);
    body->accept_const(*this);
  }

  instance->collectors.current().push(emplaced_type);
}

void TypeIdentifierResolver::property_node(const PropertyNode& node) {
  Type* prop_type;

  if (node.type) {
    TypeIdentifierResolverInstance::TypeCollectorState::Helper collector_helper(instance->collectors);
    node.type->accept_const(*this);
    auto collector = std::move(instance->collectors.current());
    if (collector.had_error) {
      instance->mark_parent_collector_error();
      return;
    }
    assert(collector.types.size() == 1);
    prop_type = collector.types[0];
  } else {
    prop_type = instance->type_store.make_fresh_type_variable_reference();
  }

  TypeIdentifier prop_name = to_type_identifier(node.name);
  const auto name_type = instance->type_store.make_constant_value(prop_name);

  instance->collectors.current().push(name_type);
  instance->collectors.current().push(prop_type);
}

void TypeIdentifierResolver::class_def_node(const ClassDefNode& node) {
  TypeIdentifier class_name = to_type_identifier(instance->def_store.get_name(node.handle));
  auto maybe_class = instance->scopes.current()->lookup_type(class_name);
  assert(maybe_class && maybe_class.value()->type->is_class());
  auto* class_type = MT_CLASS_MUT_PTR(maybe_class.value()->type);
  assert(!class_type->source);

  //  Register type with library.
  instance->library.emplace_local_class_type(node.handle, class_type);

  types::Record::Fields fields;
  for (const auto& prop : node.properties) {
    TypeIdentifierResolverInstance::TypeCollectorState::Helper collector_helper(instance->collectors);
    prop->accept_const(*this);
    auto collector = std::move(instance->collectors.current());
    if (collector.had_error) {
      instance->mark_parent_collector_error();
      return;
    }

    assert(collector.types.size() == 2);
    const auto name_type = collector.types[0];
    const auto prop_type = collector.types[1];
    fields.push_back(types::Record::Field{name_type, prop_type});
  }

  class_type->source = instance->type_store.make_record(std::move(fields));

  for (const auto& method : node.method_defs) {
    TypeIdentifierResolverInstance::TypeCollectorState::Helper collector_helper(instance->collectors);
    method->accept_const(*this);

    auto& collector = instance->collectors.current();
    assert(!collector.had_error && collector.types.size() == 1);
    auto* func_type = collector.types[0];
    auto* source_type = func_type;

    if (!func_type->is_abstraction()) {
      assert(func_type->is_scheme() && MT_SCHEME_PTR(func_type)->type->is_abstraction());
      func_type = MT_SCHEME_PTR(func_type)->type;
    }

    instance->library.method_store.add_method(class_type, MT_ABSTR_REF(*func_type), source_type);
  }
}

#if MT_ALTERNATE_TYPE_ASSSERT
void TypeIdentifierResolver::alternate_type_assertion(const TypeAssertion& assertion) {
  assert(assertion.node->represents_function_def() && assertion.has_type->is_function_type());
  const auto& function_def = static_cast<const FunctionDefNode&>(*assertion.node);
  const auto& function_type = static_cast<const FunctionTypeNode&>(*assertion.has_type);

  FunctionInputParameters input_params;
  std::vector<MatlabIdentifier> output_params;
  const Block* body = nullptr;

  instance->def_store.use<Store::ReadConst>([&](const auto& reader) {
    const FunctionDef& def = reader.at(function_def.def_handle);
    input_params = def.header.inputs;
    output_params = def.header.outputs;
    body = def.body.get();
  });

  ScopeState<const MatlabScope>::Helper matlab_scope_helper(instance->matlab_scopes, function_def.scope);
  ScopeState<TypeScope>::Helper type_scope_helper(instance->scopes, function_def.type_scope);

  auto maybe_inputs = collect_types(*this, *instance, function_type.inputs);
  auto maybe_outputs = collect_types(*this, *instance, function_type.outputs);
  assert(maybe_inputs && maybe_outputs);
  assert(maybe_inputs.value().size() == input_params.size());
  assert(maybe_outputs.value().size() == output_params.size());

  int64_t i = 0;
  for (const auto& input : input_params) {
    const auto var_handle = instance->matlab_scopes.current()->local_variables.at(input.name);
    instance->library.emplace_local_variable_type(var_handle, maybe_inputs.value()[i]);
    i++;
  }
  i = 0;
  for (const auto& output : output_params) {
    const auto var_handle = instance->matlab_scopes.current()->local_variables.at(output);
    instance->library.emplace_local_variable_type(var_handle, maybe_outputs.value()[i]);
    i++;
  }

  auto inputs = instance->type_store.make_input_destructured_tuple(maybe_inputs.rvalue());
  auto outputs = instance->type_store.make_output_destructured_tuple(maybe_outputs.rvalue());
  auto func = instance->type_store.make_abstraction(inputs, outputs);

  instance->library.emplace_local_function_type(function_def.def_handle, func);

  if (body) {
    //  Push monomorphic functions.
    BooleanState::Helper polymorphic_helper(instance->polymorphic_function_state, false);
    body->accept_const(*this);
  }

  instance->collectors.current().push(func);
}
#endif

}
