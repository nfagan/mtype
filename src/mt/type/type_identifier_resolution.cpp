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

  ParseError make_error_duplicate_method(const TypeIdentifierResolverInstance& instance, const Token& token) {
    const auto msg = "Duplicate method.";
    return ParseError(instance.source_data.source, token, msg, instance.source_data.file_descriptor);
  }

  ParseError make_error_duplicate_scheme_variable(const TypeIdentifierResolverInstance& instance,
                                                  const Token& token, const std::string& ident) {
    auto msg = "Duplicate scheme variable identifier: `" + ident + "`.";
    return ParseError(instance.source_data.source, token, msg, instance.source_data.file_descriptor);
  }

  ParseError make_error_incorrect_num_arguments_to_scheme(const TypeIdentifierResolverInstance& instance,
                                                          const Token& token) {
    auto msg = "Incorrect number of arguments to type scheme.";
    return ParseError(instance.source_data.source, token, msg, instance.source_data.file_descriptor);
  }

  ParseError make_error_type_arguments_to_non_scheme(const TypeIdentifierResolverInstance& instance,
                                                     const Token& token) {
    auto msg = "Type arguments provided to non-scheme type.";
    return ParseError(instance.source_data.source, token, msg, instance.source_data.file_descriptor);
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

void TypeIdentifierResolverInstance::push_scheme() {
  scheme_variables.emplace_back();
}

void TypeIdentifierResolverInstance::pop_scheme() {
  assert(!scheme_variables.empty());
  scheme_variables.pop_back();
}

TypeIdentifierResolverInstance::SchemeVariables& TypeIdentifierResolverInstance::current_scheme_variables() {
  assert(!scheme_variables.empty());
  return scheme_variables.back();
}

bool TypeIdentifierResolverInstance::has_scheme() const {
  return !scheme_variables.empty();
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

void TypeIdentifierResolver::inline_type(InlineType& node) {
  node.type->accept(*this);
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
  {
    instance->collectors.push();
    MT_SCOPE_EXIT {
      instance->collectors.pop();
    };

    node.has_type->accept(*this);
    auto& collector = instance->collectors.current();
    if (collector.had_error) {
      instance->mark_parent_collector_error();
      return;
    }

    assert(collector.types.size() == 1 && !node.resolved_type);
    node.resolved_type = collector.types[0];
  }

  node.node->accept(*this);
}

void TypeIdentifierResolver::type_let(TypeLet& node) {
  node.equal_to_type->accept(*this);
}

void TypeIdentifierResolver::type_given(TypeGiven& node) {
  instance->push_scheme();
  MT_SCOPE_EXIT {
    instance->pop_scheme();
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

  instance->collectors.push();
  MT_SCOPE_EXIT {
    instance->collectors.pop();
  };

  node.declaration->accept(*this);
  const auto& collector = instance->collectors.current();
  if (collector.had_error) {
    instance->mark_parent_collector_error();
    return;
  }

  assert(collector.types.size() == 1);
}

void TypeIdentifierResolver::function_type_node(FunctionTypeNode& node) {
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

  assert(!node.resolved_type);
  node.resolved_type = func;
}

void TypeIdentifierResolver::tuple_type_node(TupleTypeNode& node) {
  auto maybe_members = collect_types(*this, *instance, node.members);
  if (!maybe_members) {
    instance->collectors.current().mark_error();
    return;
  }

  auto tuple_type = instance->type_store.make_tuple(std::move(maybe_members.rvalue()));
  instance->collectors.current().push(tuple_type);
}

Type* TypeIdentifierResolver::resolve_identifier_reference(ScalarTypeNode& node) const {
  //  First check if this is a scheme variable.
  if (instance->has_scheme()) {
    const auto& scheme = instance->current_scheme_variables();
    if (scheme.variables.count(node.identifier) > 0) {
      return scheme.variables.at(node.identifier);
    }
  }

  auto* scope = instance->scopes.current();
  const auto maybe_ref = scope->lookup_type(node.identifier);

  return maybe_ref ? maybe_ref.value()->type : nullptr;
}

Type* TypeIdentifierResolver::instantiate_scheme(const types::Scheme& scheme, ScalarTypeNode& node) {
  if (scheme.parameters.size() != node.arguments.size()) {
    instance->add_error(make_error_incorrect_num_arguments_to_scheme(*instance, node.source_token));
    instance->collectors.current().mark_error();
    return nullptr;
  }

  TypePtrs args;
  args.reserve(node.arguments.size());

  for (auto& arg : node.arguments) {
    instance->collectors.push();
    MT_SCOPE_EXIT {
      instance->collectors.pop();
    };

    arg->accept(*this);
    auto& collector = instance->collectors.current();
    if (collector.had_error) {
      instance->mark_parent_collector_error();
      return nullptr;
    }

    assert(collector.types.size() == 1);
    args.push_back(collector.types[0]);
  }

  Instantiation::InstanceVars instance_vars;
  for (int64_t i = 0; i < int64_t(args.size()); i++) {
    const auto source = scheme.parameters[i];
    const auto map_to = args[i];
    instance_vars[source] = map_to;
  }

  Instantiation instantiation(instance->type_store);
  return instantiation.instantiate(scheme, instance_vars);
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
    auto maybe_result_type = instantiate_scheme(MT_SCHEME_REF(*type), node);
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
    instance->collectors.push();
    MT_SCOPE_EXIT {
      instance->collectors.pop();
    };
    field.type->accept(*this);

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

  instance->collectors.push();
  MT_SCOPE_EXIT {
    instance->collectors.pop();
  };

  node.maybe_method.type->accept(*this);
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

void TypeIdentifierResolver::function_def_node(FunctionDefNode& node) {
  instance->matlab_scopes.push(node.scope);
  instance->scopes.push(node.type_scope);

  MT_SCOPE_EXIT {
    instance->matlab_scopes.pop();
    instance->scopes.pop();
  };

  MatlabIdentifier function_name;
  Block* body = nullptr;
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
    instance->collectors.push();
    //  Push monomorphic functions.
    instance->polymorphic_function_state.push(false);

    MT_SCOPE_EXIT {
      instance->collectors.pop();
      instance->polymorphic_function_state.pop();
    };

    body->accept(*this);
  }

  instance->collectors.current().push(emplaced_type);
}

void TypeIdentifierResolver::property_node(PropertyNode& node) {
  Type* prop_type = nullptr;

  if (node.type) {
    instance->collectors.push();
    MT_SCOPE_EXIT {
      instance->collectors.pop();
    };
    node.type->accept(*this);
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

void TypeIdentifierResolver::method_node(MethodNode& node) {
  if (node.type) {
    instance->collectors.push();
    MT_SCOPE_EXIT {
      instance->collectors.pop();
    };
    node.type->accept(*this);
    auto& collector = instance->collectors.current();
    if (collector.had_error) {
      instance->mark_parent_collector_error();
      return;
    }
    assert(collector.types.size() == 1 && !node.resolved_type);
    node.resolved_type = collector.types[0];
  }

  node.def->accept(*this);
}

void TypeIdentifierResolver::class_def_node(ClassDefNode& node) {
  TypeIdentifier class_name = to_type_identifier(instance->def_store.get_name(node.handle));
  auto maybe_class = instance->scopes.current()->lookup_type(class_name);
  assert(maybe_class && maybe_class.value()->type->is_class());
  auto* class_type = MT_CLASS_MUT_PTR(maybe_class.value()->type);
  assert(!class_type->source);

  //  Register type with library.
  instance->library.emplace_local_class_type(node.handle, class_type);

  types::Record::Fields fields;
  for (const auto& prop : node.properties) {
    instance->collectors.push();
    MT_SCOPE_EXIT {
      instance->collectors.pop();
    };

    prop->accept(*this);
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
    instance->collectors.push();
    MT_SCOPE_EXIT {
      instance->collectors.pop();
    };

    method->accept(*this);

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

}
