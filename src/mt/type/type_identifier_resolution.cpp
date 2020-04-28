#include "type_identifier_resolution.hpp"
#include "type_scope.hpp"
#include "../ast.hpp"
#include "../store.hpp"
#include "../string.hpp"
#include "library.hpp"
#include "type_store.hpp"
#include "instance.hpp"
#include "debug.hpp"
#include <cassert>

namespace mt {

namespace {
  void visit_array(TypeIdentifierResolver& resolver, std::vector<BoxedType>& types) {
    for (auto& type : types) {
      type->accept(resolver);
    }
  }

  Optional<TypePtrs> collect_types(TypeIdentifierResolver& resolver,
                                   TypeIdentifierResolverInstance& instance,
                                   std::vector<BoxedType>& types) {
    instance.collectors.push();
    MT_SCOPE_EXIT {
      instance.collectors.pop();
    };

    visit_array(resolver, types);
    auto& collector = instance.collectors.current();
    if (collector.had_error) {
      instance.mark_parent_collector_error();
      return NullOpt{};
    } else {
      return Optional<TypePtrs>(std::move(collector.types));
    }
  }

  Optional<TypePtrs> collect_types(TypeIdentifierResolver& resolver,
                                   TypeIdentifierResolverInstance& instance,
                                   AstNode* node) {
    instance.collectors.push();
    MT_SCOPE_EXIT {
      instance.collectors.pop();
    };

    node->accept(resolver);

    auto& collector = instance.collectors.current();
    if (collector.had_error) {
      instance.mark_parent_collector_error();
      return NullOpt{};
    } else {
      return Optional<TypePtrs>(std::move(collector.types));
    }
  }

  Optional<Type*> collect_one_type(TypeIdentifierResolver& resolver,
                                   TypeIdentifierResolverInstance& instance,
                                   AstNode* node) {
    auto maybe_ptrs = collect_types(resolver, instance, node);
    if (!maybe_ptrs) {
      return NullOpt{};
    } else {
      //  It's a bug if more or fewer than one type is present.
      assert(maybe_ptrs.value().size() == 1);
      return Optional<Type*>(maybe_ptrs.value()[0]);
    }
  }

  ParseSourceData lookup_source_data(const TokenSourceMap& source_map, const Token& token) {
    auto expect_source_data = source_map.lookup(token);
    assert(expect_source_data);
    return expect_source_data.value();
  }

  ParseError make_error_unresolved_type_identifier(const TypeIdentifierResolverInstance& instance,
                                                   const Token& token, const std::string& ident) {
    const auto source_data = lookup_source_data(instance.source_map, token);
    auto msg = "Unresolved type identifier: `" + ident + "`.";
    return ParseError(source_data.source, token, msg, source_data.file_descriptor);
  }

  ParseError make_error_duplicate_method(const TypeIdentifierResolverInstance& instance, const Token& token) {
    const auto source_data = lookup_source_data(instance.source_map, token);
    const auto msg = "Duplicate method.";
    return ParseError(source_data.source, token, msg, source_data.file_descriptor);
  }

  ParseError make_error_duplicate_scheme_variable(const TypeIdentifierResolverInstance& instance,
                                                  const Token& token, const std::string& ident) {
    const auto source_data = lookup_source_data(instance.source_map, token);
    auto msg = "Duplicate scheme variable identifier: `" + ident + "`.";
    return ParseError(source_data.source, token, msg, source_data.file_descriptor);
  }

  ParseError make_error_incorrect_num_arguments_to_scheme(const TypeIdentifierResolverInstance& instance,
                                                          const Token& token) {
    const auto source_data = lookup_source_data(instance.source_map, token);
    auto msg = "Incorrect number of arguments to type scheme.";
    return ParseError(source_data.source, token, msg, source_data.file_descriptor);
  }

  ParseError make_error_type_arguments_to_non_scheme(const TypeIdentifierResolverInstance& instance,
                                                     const Token& token) {
    const auto source_data = lookup_source_data(instance.source_map, token);
    auto msg = "Type arguments provided to non-scheme type.";
    return ParseError(source_data.source, token, msg, source_data.file_descriptor);
  }

  ParseError make_error_wrong_num_params_in_func_type(const TypeIdentifierResolverInstance& instance,
                                                      const Token& token) {
    const auto source_data = lookup_source_data(instance.source_map, token);
    auto msg = "Number of parameters in function type does not match its definition.";
    return ParseError(source_data.source, token, msg, source_data.file_descriptor);
  }

  ParseError make_error_expected_function_type(const TypeIdentifierResolverInstance& instance,
                                               const Token& token) {
    const auto source_data = lookup_source_data(instance.source_map, token);
    auto msg = "Expected a function type.";
    return ParseError(source_data.source, token, msg, source_data.file_descriptor);
  }

  ParseError make_error_duplicate_function(const TypeIdentifierResolverInstance& instance,
                                           const Token& token) {
    const auto source_data = lookup_source_data(instance.source_map, token);
    auto msg = "Duplicate function.";
    return ParseError(source_data.source, token, msg, source_data.file_descriptor);
  }

  ParseError make_error_duplicate_class(const TypeIdentifierResolverInstance& instance,
                                        const Token& token) {
    const auto source_data = lookup_source_data(instance.source_map, token);
    auto msg = "Duplicate class.";
    return ParseError(source_data.source, token, msg, source_data.file_descriptor);
  }
}

/*
 * TypeIdentifierResolverInstance::UnresolvedIdentifier
 */

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

TypeIdentifierResolverInstance::PendingScheme::PendingScheme(const types::Scheme* scheme,
                                                             TypePtrs args,
                                                             types::Alias* target) :
scheme(scheme), arguments(std::move(args)), target(target) {
  //
}

void TypeIdentifierResolverInstance::PendingScheme::instantiate(TypeStore& store) const {
  assert(arguments.size() == scheme->parameters.size());
  assert(!target->source);

  Instantiation::InstanceVars instance_vars;
  for (int64_t i = 0; i < int64_t(arguments.size()); i++) {
    const auto source = scheme->parameters[i];
    const auto map_to = arguments[i];
    instance_vars[source] = map_to;
  }

  Instantiation instantiation(store);
  target->source = instantiation.instantiate(*scheme, instance_vars);
}

/*
 * TypeIdentifierResolverInstance
 */

TypeIdentifierResolverInstance::TypeIdentifierResolverInstance(TypeStore& type_store,
                                                               Library& library,
                                                               const Store& def_store,
                                                               const StringRegistry& string_registry,
                                                               const TokenSourceMap& source_map) :
type_store(type_store), library(library), def_store(def_store), string_registry(string_registry),
source_map(source_map) {
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

void TypeIdentifierResolverInstance::push_scheme_variables() {
  scheme_variables.emplace_back();
}

void TypeIdentifierResolverInstance::pop_scheme_variables() {
  assert(!scheme_variables.empty());
  scheme_variables.pop_back();
}

TypeIdentifierResolverInstance::SchemeVariables& TypeIdentifierResolverInstance::current_scheme_variables() {
  assert(!scheme_variables.empty());
  return scheme_variables.back();
}

bool TypeIdentifierResolverInstance::has_scheme_variables() const {
  return !scheme_variables.empty();
}

void TypeIdentifierResolverInstance::push_pending_scheme(PendingScheme&& pending_scheme) {
  pending_schemes.push_back(std::move(pending_scheme));
}

void TypeIdentifierResolverInstance::push_presumed_type(Type* type) {
  presumed_types.push_back(type);
}

void TypeIdentifierResolverInstance::pop_presumed_type() {
  assert(!presumed_types.empty());
  presumed_types.pop_back();
}

Type* TypeIdentifierResolverInstance::presumed_type() const {
  return presumed_types.empty() ? nullptr : presumed_types.back();
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

void TypeIdentifierResolver::root_block(RootBlock& block) {
  //  Push root type scope.
  instance->scopes.push(block.type_scope);
  instance->matlab_scopes.push(block.scope);
  instance->collectors.push();
  //  Push monomorphic functions.
  instance->polymorphic_function_state.push(false);

  MT_SCOPE_EXIT {
    instance->scopes.pop();
    instance->matlab_scopes.pop();
    instance->collectors.pop();
    instance->polymorphic_function_state.pop();
  };

  block.block->accept(*this);
}

void TypeIdentifierResolver::block(Block& block) {
  for (const auto& node : block.nodes) {
    node->accept(*this);
  }
}

/*
 * TypeAnnot
 */

void TypeIdentifierResolver::type_annot_macro(TypeAnnotMacro& macro) {
  macro.annotation->accept(*this);
}

void TypeIdentifierResolver::type_begin(TypeBegin& begin) {
  instance->export_state.dispatched_push(begin.is_exported);
  MT_SCOPE_EXIT {
    instance->export_state.pop_state();
  };

  for (const auto& node : begin.contents) {
    node->accept(*this);
  }
}

void TypeIdentifierResolver::type_assertion(TypeAssertion& node) {
  auto maybe_resolved_type = collect_one_type(*this, *instance, node.has_type.get());
  if (!maybe_resolved_type) {
    return;
  } else {
    node.resolved_type = maybe_resolved_type.value();
  }

  instance->push_presumed_type(node.resolved_type);
  MT_SCOPE_EXIT {
    instance->pop_presumed_type();
  };

  node.node->accept(*this);
}

void TypeIdentifierResolver::type_let(TypeLet& node) {
  auto maybe_type = collect_one_type(*this, *instance, node.equal_to_type.get());
  if (!maybe_type) {
    return;
  }

  assert(!node.type_alias->source);
  node.type_alias->source = maybe_type.value();
}

void TypeIdentifierResolver::scheme_type_node(SchemeTypeNode& node) {
  instance->push_scheme_variables();
  MT_SCOPE_EXIT {
    instance->pop_scheme_variables();
  };

  auto& scheme_variables = instance->current_scheme_variables();
  assert(node.scheme->parameters.size() == node.identifiers.size());

  for (int64_t i = 0; i < int64_t(node.identifiers.size()); i++) {
    //  Duplicate variable.
    const auto& ident = node.identifiers[i];
    TypeIdentifier type_ident(ident);

    if (scheme_variables.variables.count(type_ident) > 0) {
      const auto ident_str = instance->string_registry.at(ident);
      instance->add_error(make_error_duplicate_scheme_variable(*instance, node.source_token, ident_str));
      return;
    }

    const auto tvar = node.scheme->parameters[i];
    scheme_variables.variables[type_ident] = tvar;
  }

  auto maybe_source_type = collect_one_type(*this, *instance, node.declaration.get());
  if (!maybe_source_type) {
    return;
  }

  assert(!node.scheme->type);
  node.scheme->type = maybe_source_type.value();

  instance->collectors.current().push(node.scheme);
}

void TypeIdentifierResolver::function_type_node(FunctionTypeNode& node) {
  auto maybe_inputs = collect_types(*this, *instance, node.inputs);
  if (!maybe_inputs) {
    return;
  }

  auto maybe_outputs = collect_types(*this, *instance, node.outputs);
  if (!maybe_outputs) {
    return;
  }

  auto inputs = instance->type_store.make_input_destructured_tuple(std::move(maybe_inputs.rvalue()));
  auto outputs = instance->type_store.make_output_destructured_tuple(std::move(maybe_outputs.rvalue()));
  auto func = instance->type_store.make_abstraction(inputs, outputs);
  instance->collectors.current().push(func);

  assert(!node.resolved_type);
  node.resolved_type = func;
}

void TypeIdentifierResolver::tuple_type_node(TupleTypeNode& node) {
  auto maybe_members = collect_types(*this, *instance, node.members);
  if (!maybe_members) {
    return;
  }

  auto tuple_type = instance->type_store.make_tuple(std::move(maybe_members.rvalue()));
  instance->collectors.current().push(tuple_type);
}

void TypeIdentifierResolver::union_type_node(UnionTypeNode& node) {
  auto maybe_members = collect_types(*this, *instance, node.members);
  if (!maybe_members) {
    return;
  }

  auto union_type = instance->type_store.make_union(std::move(maybe_members.rvalue()));
  instance->collectors.current().push(union_type);
}

void TypeIdentifierResolver::list_type_node(ListTypeNode& node) {
  auto maybe_pattern = collect_types(*this, *instance, node.pattern);
  if (!maybe_pattern) {
    return;
  }

  auto list_type = instance->type_store.make_list(std::move(maybe_pattern.rvalue()));
  instance->collectors.current().push(list_type);
}

Type* TypeIdentifierResolver::resolve_identifier_reference(ScalarTypeNode& node) const {
  //  First check if this is a scheme variable.
  if (instance->has_scheme_variables()) {
    const auto& scheme = instance->current_scheme_variables();
    if (scheme.variables.count(node.identifier) > 0) {
      return scheme.variables.at(node.identifier);
    }
  }

  auto* scope = instance->scopes.current();
  const auto maybe_ref = scope->lookup_type(node.identifier);

  return maybe_ref ? maybe_ref.value()->type : nullptr;
}

Type* TypeIdentifierResolver::handle_pending_scheme(const types::Scheme* scheme, ScalarTypeNode& node) {
  if (scheme->parameters.size() != node.arguments.size()) {
    instance->add_error(make_error_incorrect_num_arguments_to_scheme(*instance, node.source_token));
    instance->collectors.current().mark_error();
    return nullptr;
  }

  TypePtrs args;
  args.reserve(node.arguments.size());

  for (auto& arg : node.arguments) {
    auto maybe_type = collect_one_type(*this, *instance, arg.get());
    if (!maybe_type) {
      return nullptr;
    } else {
      args.push_back(maybe_type.value());
    }
  }

  //  We can't directly instantiate a scheme here, because the scheme's source might be an imported
  //  type which has been declared, but not yet defined.
  auto alias = instance->type_store.make_alias();
  TypeIdentifierResolverInstance::PendingScheme pending_scheme(scheme, std::move(args), alias);
  instance->push_pending_scheme(std::move(pending_scheme));

  return alias;
}

void TypeIdentifierResolver::scalar_type_node(ScalarTypeNode& node) {
  auto type = resolve_identifier_reference(node);

  if (!type) {
    const auto str_ident = instance->string_registry.at(node.identifier.full_name());
    instance->add_error(make_error_unresolved_type_identifier(*instance, node.source_token, str_ident));
    instance->add_unresolved_identifier(node.identifier, instance->scopes.current());
    instance->collectors.current().mark_error();
    return;
  }

  auto result_type = type;

  if (type->is_scheme()) {
    auto maybe_result_type = handle_pending_scheme(MT_SCHEME_PTR(type), node);
    if (!maybe_result_type) {
      return;
    } else {
      result_type = maybe_result_type;
    }
  } else if (!node.arguments.empty()) {
    instance->add_error(make_error_type_arguments_to_non_scheme(*instance, node.source_token));
    instance->collectors.current().mark_error();
    return;
  }

  instance->collectors.current().push(result_type);
}

void TypeIdentifierResolver::infer_type_node(InferTypeNode& node) {
  assert(node.type);
  instance->collectors.current().push(node.type);
}

void TypeIdentifierResolver::fun_type_node(FunTypeNode& node) {
  //  Push polymorphic functions
  instance->polymorphic_function_state.push(true);
  MT_SCOPE_EXIT {
    instance->polymorphic_function_state.pop();
  };

  node.definition->accept(*this);
}

void TypeIdentifierResolver::record_type_node(RecordTypeNode& node) {
  for (auto& field : node.fields) {
    auto maybe_field_type = collect_one_type(*this, *instance, field.type.get());
    if (!maybe_field_type) {
      return;
    }

    auto field_name = instance->type_store.make_constant_value(field.name);
    auto field_type = maybe_field_type.value();

    node.type->fields.push_back(types::Record::Field{field_name, field_type});
  }

  instance->collectors.current().push(node.type);
}

void TypeIdentifierResolver::declare_type_node(DeclareTypeNode& node) {
  switch (node.kind) {
    case DeclareTypeNode::Kind::scalar:
      scalar_type_declaration(node);
      break;
    case DeclareTypeNode::Kind::method:
      method_type_declaration(node);
      break;
  }
}

void TypeIdentifierResolver::scalar_type_declaration(DeclareTypeNode& node) {
  auto maybe_scalar = instance->scopes.current()->lookup_type(node.identifier);
  assert(maybe_scalar && maybe_scalar.value()->type);
  if (maybe_scalar) {
    //
  }
}

void TypeIdentifierResolver::method_type_declaration(DeclareTypeNode& node) {
  auto& library = instance->library;
  auto& method_store = library.method_store;

  auto maybe_class = library.lookup_class(node.identifier);
  if (!maybe_class) {
    instance->add_unresolved_identifier(node.identifier, instance->scopes.current());
    return;
  }

  auto maybe_collected_type = collect_one_type(*this, *instance, node.maybe_method.type.get());
  if (!maybe_collected_type) {
    return;
  }

  auto collected_type = maybe_collected_type.value();
  assert(collected_type->is_abstraction());
  auto& method = MT_ABSTR_MUT_REF(*collected_type);

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

void TypeIdentifierResolver::declare_function_type_node(DeclareFunctionTypeNode& node) {
  auto maybe_type = collect_one_type(*this, *instance, node.type.get());
  if (!maybe_type) {
    return;
  }

  auto type = maybe_type.value();
  assert(type->scheme_source()->is_abstraction());
  auto& abstr = MT_ABSTR_MUT_REF(*type->scheme_source());
  abstr.assign_kind(to_matlab_identifier(node.identifier));

  bool success = instance->library.emplace_declared_function_type(abstr, type);
  if (!success) {
    instance->add_error(make_error_duplicate_function(*instance, node.source_token));
    instance->collectors.current().mark_error();
  }
}

void TypeIdentifierResolver::namespace_type_node(NamespaceTypeNode& node) {
  instance->namespace_state.push(node.identifier);
  MT_SCOPE_EXIT {
    instance->namespace_state.pop();
  };

  for (const auto& enclosed : node.enclosing) {
    enclosed->accept(*this);
  }
}

/*
 * Def
 */

namespace {
  void gather_function_parameters(const TypeIdentifierResolverInstance& instance,
                                  const FunctionParameters& params,
                                  TypePtrs& current_types,
                                  TypePtrs& all_types) {
    const auto& matlab_scope = *instance.matlab_scopes.current();

    for (const auto& arg : params) {
      auto tvar = (arg.is_ignored() || arg.is_part_of_decl()) ?
        instance.type_store.make_fresh_type_variable_reference() :
        instance.library.require_local_variable_type(matlab_scope.local_variables.at(arg.name));

      current_types.push_back(tvar);
      all_types.push_back(tvar);
    }
  }

  void emplace_function_parameters(const TypeIdentifierResolverInstance& instance,
                                   const FunctionParameters& params,
                                   const TypePtrs& args) {
    const auto& matlab_scope = *instance.matlab_scopes.current();
    const int64_t num_types = params.size();
    assert(params.size() == args.size());

    for (int64_t i = 0; i < num_types; i++) {
      const auto& param = params[i];
      if (!param.is_ignored() && !param.is_part_of_decl()) {
        const auto var_handle = matlab_scope.local_variables.at(param.name);
        instance.library.emplace_local_variable_type(var_handle, args[i]);
      }
    }
  }

  Type* function_header_presumed_type(TypeIdentifierResolverInstance& instance,
                                      const FunctionHeader& header,
                                      Type* presumed_type,
                                      const FunctionReferenceHandle& ref_handle,
                                      const Token& source_token) {
    auto maybe_source_type = presumed_type->scheme_source();

    if (!maybe_source_type->is_abstraction()) {
      instance.add_error(make_error_expected_function_type(instance, source_token));
      instance.collectors.current().mark_error();
      return nullptr;
    }

    auto& abstr = MT_ABSTR_MUT_REF(*maybe_source_type);
    assert(abstr.inputs && abstr.inputs->is_destructured_tuple() &&
           abstr.outputs && abstr.outputs->is_destructured_tuple());

    const auto& inputs = MT_DT_REF(*abstr.inputs);
    const auto& outputs = MT_DT_REF(*abstr.outputs);

    if (inputs.size() != header.num_inputs() ||
        outputs.size() != header.num_outputs()) {
      instance.collectors.current().mark_error();
      instance.add_error(make_error_wrong_num_params_in_func_type(instance, source_token));
      return nullptr;
    }

    emplace_function_parameters(instance, header.inputs, inputs.members);
    emplace_function_parameters(instance, header.outputs, outputs.members);

    abstr.assign_kind(header.name, ref_handle);

    return presumed_type;
  }

  Type* function_header_new_type(TypeIdentifierResolverInstance& instance,
                                 const FunctionDefHandle& def_handle,
                                 const FunctionReferenceHandle& ref_handle) {
    TypePtrs inputs;
    TypePtrs outputs;
    TypePtrs all_parameters;

    MatlabIdentifier function_name;

    instance.def_store.use<Store::ReadConst>([&](const auto& reader) {
      const auto& def = reader.at(def_handle);
      function_name = def.header.name;

      gather_function_parameters(instance, def.header.inputs, inputs, all_parameters);
      gather_function_parameters(instance, def.header.outputs, outputs, all_parameters);
    });

    auto& store = instance.type_store;
    const auto input = store.make_input_destructured_tuple(std::move(inputs));
    const auto output = store.make_output_destructured_tuple(std::move(outputs));
    auto abstr = store.make_abstraction(function_name, ref_handle, input, output);

    Type* result_type = abstr;
    if (instance.polymorphic_function_state.value()) {
      result_type = store.make_scheme(abstr, std::move(all_parameters));
    }

    return result_type;
  }
}

void TypeIdentifierResolver::function_def_node(FunctionDefNode& node) {
  instance->matlab_scopes.push(node.scope);
  instance->scopes.push(node.type_scope);

  MT_SCOPE_EXIT {
    instance->matlab_scopes.pop();
    instance->scopes.pop();
  };

  Type* emplaced_type = nullptr;

  if (instance->presumed_type()) {
    instance->def_store.use<Store::ReadConst>([&](const auto& reader) {
      const auto& def = reader.at(node.def_handle);
      emplaced_type = function_header_presumed_type(*instance, def.header,
        instance->presumed_type(), node.ref_handle, node.source_token);
    });
  } else {
    emplaced_type = function_header_new_type(*instance, node.def_handle, node.ref_handle);
  }

  if (!emplaced_type) {
    return;
  }

  instance->library.emplace_local_function_type(node.def_handle, emplaced_type);
  Block* body = instance->def_store.get_block(node.def_handle);

  if (body) {
    instance->collectors.push();
    //  Push monomorphic functions.
    instance->polymorphic_function_state.push(emplaced_type->is_scheme());
    instance->push_presumed_type(nullptr);

    MT_SCOPE_EXIT {
      instance->collectors.pop();
      instance->polymorphic_function_state.pop();
      instance->pop_presumed_type();
    };

    body->accept(*this);
  }

  instance->collectors.current().push(emplaced_type);
}

void TypeIdentifierResolver::property_node(PropertyNode& node) {
  Type* prop_type = nullptr;

  if (node.type) {
    auto maybe_prop_type = collect_one_type(*this, *instance, node.type.get());
    if (!maybe_prop_type) {
      return;
    } else {
      prop_type = maybe_prop_type.value();
    }
  } else {
    prop_type = instance->type_store.make_fresh_type_variable_reference();
  }

  TypeIdentifier prop_name = to_type_identifier(node.name);
  const auto name_type = instance->type_store.make_constant_value(prop_name);

  instance->collectors.current().push(name_type);
  instance->collectors.current().push(prop_type);
}

void TypeIdentifierResolver::method_node(MethodNode& node) {
  if (node.type) {
    auto maybe_method_type = collect_one_type(*this, *instance, node.type.get());
    if (!maybe_method_type) {
      return;
    }
    assert(!node.resolved_type);
    node.resolved_type = maybe_method_type.value();
  }

  {
    instance->push_presumed_type(node.resolved_type);
    MT_SCOPE_EXIT {
      instance->pop_presumed_type();
    };

    auto maybe_emplaced_type = collect_one_type(*this, *instance, node.def.get());
    if (!maybe_emplaced_type) {
      return;
    } else if (!node.resolved_type) {
      node.resolved_type = maybe_emplaced_type.value();
    }
  }

  if (node.external_block) {
    auto maybe_types = collect_types(*this, *instance, node.external_block.get());
    if (!maybe_types) {
      return;
    }
  }

  assert(node.resolved_type);
  instance->collectors.current().push(node.resolved_type);
}

void TypeIdentifierResolver::class_def_node(ClassDefNode& node) {
  TypeIdentifier class_name = to_type_identifier(instance->def_store.get_name(node.handle));
  auto maybe_class = instance->scopes.current()->lookup_type(class_name);
  assert(maybe_class && maybe_class.value()->type->is_class());
  auto* class_type = MT_CLASS_MUT_PTR(maybe_class.value()->type);
  assert(!class_type->source);

  //  Register type with library.
  bool register_success =
    instance->library.emplace_local_class_type(node.handle, class_type);

  if (!register_success) {
    instance->add_error(make_error_duplicate_class(*instance, node.source_token));
    instance->collectors.current().mark_error();
    return;
  }

  types::Record::Fields fields;
  for (const auto& prop : node.properties) {
    auto maybe_types = collect_types(*this, *instance, prop.get());
    if (!maybe_types) {
      return;
    }

    const auto& types = maybe_types.value();
    assert(types.size() == 2);
    const auto name_type = types[0];
    const auto prop_type = types[1];
    fields.push_back(types::Record::Field{name_type, prop_type});
  }

  class_type->source = instance->type_store.make_record(std::move(fields));

  for (const auto& method : node.method_defs) {
    auto maybe_method_type = collect_one_type(*this, *instance, method.get());
    if (!maybe_method_type) {
      return;
    }

    auto* func_type = maybe_method_type.value();
    auto* source_type = func_type;

    if (!func_type->is_abstraction()) {
      assert(func_type->is_scheme() && MT_SCHEME_PTR(func_type)->type->is_abstraction());
      func_type = MT_SCHEME_PTR(func_type)->type;
    }

    instance->library.method_store.add_method(class_type, MT_ABSTR_REF(*func_type), source_type);
  }
}

}
