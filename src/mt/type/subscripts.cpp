#include "subscripts.hpp"
#include "unification.hpp"
#include "library.hpp"
#include "debug.hpp"
#include <cassert>

#define MT_SHOW1(msg, a) \
  std::cout << (msg); \
  unifier.type_printer().show((a)); \
  std::cout << std::endl

#define MT_SHOW2(msg, a, b) \
  std::cout << (msg) << std::endl; \
  unifier.type_printer().show2((a), (b)); \
  std::cout << std::endl

namespace mt {

SubscriptHandler::SubscriptHandler(mt::Unifier& unifier) : unifier(unifier), library(unifier.library) {
  //
}

#if 0
void SubscriptHandler::maybe_unify_subscript(Type* source, TermRef term, types::Subscript& sub) {
  const auto arg0 = sub.principal_argument;

  if (unifier.is_known_subscript_type(sub.principal_argument)) {
    maybe_unify_known_subscript_type(source, term, sub);

  } else if (arg0->is_abstraction()) {
    //  a() where a is a variable, discovered to be a function reference.
    maybe_unify_function_call_subscript(source, term, MT_ABSTR_REF(*arg0), sub);

  } else if (arg0->is_scheme()) {
    maybe_unify_anonymous_function_call_subscript(source, term, MT_SCHEME_REF(*arg0), sub);

  } else if (arg0->is_class()) {
    maybe_unify_subscripted_class(source, term, MT_CLASS_REF(*arg0), sub);

  } else if (!arg0->is_variable() && !arg0->is_parameters()) {
//    MT_SHOW1("Tried to unify subscript with principal arg: ", sub.principal_argument);
  }
}
#else

void SubscriptHandler::maybe_unify_subscript(Type* source, TermRef term, types::Subscript& sub) {
  const auto arg0 = sub.principal_argument;

  if (unifier.is_visited_type(source) || !unifier.is_concrete_argument(sub.principal_argument)) {
    return;

  } else if (arg0->is_abstraction()) {
    function_call_subscript(source, term, MT_ABSTR_REF(*arg0), sub);

  } else if (arg0->is_scheme() && MT_SCHEME_REF(*arg0).type->is_abstraction()) {
    anonymous_function_call_subscript(source, term, MT_SCHEME_REF(*arg0), sub);

  } else {
    maybe_unify_non_function_subscript(source, term, sub);
  }
}

void SubscriptHandler::maybe_unify_non_function_subscript(Type* source, TermRef term, types::Subscript& sub) {
  assert(!sub.subscripts.empty());
  const auto& arg0 = sub.principal_argument;
  const auto& sub0 = sub.subscripts[0];

  if (!unifier.are_concrete_arguments(sub0.arguments)) {
    return;
  }

  unifier.register_visited_type(source);

  if (has_custom_subsref_method(arg0)) {
    //  @TODO: Custom subsref implementations not yet handled.
    unifier.add_error(unifier.make_unhandled_custom_subscripts_error(term.source_token, arg0));
    return;

  } else if (!are_valid_subscript_arguments(arg0, sub0)) {
    unifier.add_error(unifier.make_unresolved_function_error(term.source_token, source));
    return;
  }

  Type* next_arg0 = nullptr;

  if (sub0.is_parens()) {
    next_arg0 = arg0;

  } else if (sub0.is_period() && arg0->is_class() && MT_CLASS_PTR(arg0)->source->is_record()) {
    const auto& class_type = MT_CLASS_REF(*arg0);
    const auto& record_type = MT_RECORD_REF(*class_type.source);
    next_arg0 = record_period_subscript(arg0, term.source_token, sub0, record_type);

  } else if (sub0.is_period() && arg0->is_record()) {
    next_arg0 = record_period_subscript(arg0, term.source_token, sub0, MT_RECORD_REF(*arg0));

  } else if (sub0.is_brace() && arg0->is_tuple()) {
    next_arg0 = tuple_brace_subscript(source, term.source_token, MT_TUPLE_REF(*arg0));
  }

  if (!next_arg0) {
    return;
  }

  auto& store = unifier.store;
  auto& substitution = unifier.substitution;

  if (sub.subscripts.size() == 1) {
    auto lhs_term = make_term(term.source_token, sub.outputs);
    auto rhs_term = make_term(term.source_token, next_arg0);
    substitution->push_type_equation(make_eq(lhs_term, rhs_term));

  } else {
    sub.subscripts.erase(sub.subscripts.begin());
    sub.principal_argument = next_arg0;
    unifier.unregister_visited_type(source);

    auto rhs_term = make_term(term.source_token, store.make_fresh_type_variable_reference());
    substitution->push_type_equation(make_eq(term, rhs_term));
  }
}

#endif

Type* SubscriptHandler::tuple_brace_subscript(const Type* source,
                                              const Token* source_token,
                                              const types::Tuple& arg0) {
  const auto& members = arg0.members;
  if (members.empty()) {
    unifier.add_error(unifier.make_unresolved_function_error(source_token, source));
    return nullptr;
  }

  const auto& mem0 = members[0];
  for (int64_t i = 1; i < int64_t(members.size()); i++) {
    auto lhs_term = make_term(source_token, mem0);
    auto rhs_term = make_term(source_token, members[i]);

    unifier.substitution->push_type_equation(make_eq(lhs_term, rhs_term));
  }

  return mem0;
}

Type* SubscriptHandler::record_period_subscript(Type* record_source, const Token* source_token,
                                                const types::Subscript::Sub& sub0,
                                                const types::Record& arg0) {
  assert(sub0.arguments.size() == 1 && "Expected 1 argument.");
  const auto& sub_arg0 = sub0.arguments[0];
  if (!sub_arg0->is_constant_value()) {
    unifier.add_error(unifier.make_non_constant_field_reference_expr_error(source_token, sub_arg0));
    return nullptr;
  }

  const auto& cv = MT_CONST_VAL_REF(*sub_arg0);
  const auto maybe_field = arg0.find_field(cv);
  if (!maybe_field) {
    unifier.add_error(unifier.make_reference_to_non_existent_field_error(source_token, record_source, sub_arg0));
    return nullptr;
  }

  return maybe_field->type;
}

bool SubscriptHandler::has_custom_subsref_method(const Type* arg0) const {
  const auto maybe_class_type = library.class_for_type(arg0);
  if (maybe_class_type) {
    const auto subsref_ident = MatlabIdentifier(library.special_identifiers.subsref);
    return library.method_store.has_named_method(maybe_class_type.value(), subsref_ident);
  } else {
    return false;
  }
}

bool SubscriptHandler::are_valid_subscript_arguments(const Type* arg0, const types::Subscript::Sub& sub) const {
  if (sub.is_brace() && !arg0->is_tuple()) {
    return false;

  } else if (sub.is_period() && !(arg0->is_record() || arg0->is_class())) {
    return false;

  } else if (sub.is_parens() || sub.is_brace()) {
    return arguments_have_subsindex_defined(sub.arguments);
  }

  return true;
}

bool SubscriptHandler::arguments_have_subsindex_defined(const TypePtrs& arguments) const {
  const auto& method_store = library.method_store;
  const auto subsindex = MatlabIdentifier(library.special_identifiers.subsindex);

  for (const auto& arg : arguments) {
    const auto maybe_lookup = types::DestructuredTuple::type_or_first_non_destructured_tuple_member(arg);
    if (!maybe_lookup) {
      return false;
    }

    auto maybe_class = library.class_for_type(maybe_lookup.value());
    if (!maybe_class || !method_store.has_named_method(maybe_class.value(), subsindex)) {
      return false;
    }
  }

  return true;
}

void SubscriptHandler::anonymous_function_call_subscript(Type* source, TermRef term,
                                                         const types::Scheme& scheme,
                                                         types::Subscript& sub) {
  if (unifier.is_visited_type(source)) {
    return;
  }

  if (sub.subscripts.size() != 1 || sub.subscripts[0].method != SubscriptMethod::parens) {
    unifier.register_visited_type(source);
    unifier.add_error(unifier.make_invalid_function_invocation_error(term.source_token, sub.principal_argument));
    return;
  }

  const auto& sub0 = sub.subscripts[0];

  if (!unifier.are_concrete_arguments(sub0.arguments)) {
    return;
  }

  const auto instance_handle = unifier.instantiate(scheme);
  assert(instance_handle->is_abstraction());
  const auto& source_func = MT_ABSTR_REF(*instance_handle);

  const auto sub_lhs_term = make_term(term.source_token, sub.outputs);
  const auto sub_rhs_term = make_term(term.source_token, source_func.outputs);
  unifier.substitution->push_type_equation(make_eq(sub_lhs_term, sub_rhs_term));

  auto copy_args = sub0.arguments;
  auto new_inputs = unifier.store.make_rvalue_destructured_tuple(std::move(copy_args));
  const auto arg_lhs_term = make_term(term.source_token, new_inputs);
  const auto arg_rhs_term = make_term(term.source_token, source_func.inputs);
  unifier.substitution->push_type_equation(make_eq(arg_lhs_term, arg_rhs_term));

  unifier.register_visited_type(source);
}

void SubscriptHandler::function_call_subscript(Type* source, TermRef term,
                                               const types::Abstraction& source_func,
                                               const types::Subscript& sub) {
  if (unifier.registered_funcs.count(source) > 0) {
    return;
  }

  if (sub.subscripts.size() != 1 || sub.subscripts[0].method != SubscriptMethod::parens) {
    unifier.registered_funcs[source] = true;
    unifier.add_error(unifier.make_invalid_function_invocation_error(term.source_token, sub.principal_argument));
    return;
  }

  const auto& sub0 = sub.subscripts[0];

  if (!unifier.are_concrete_arguments(sub0.arguments)) {
    return;
  }

  auto args_copy = sub0.arguments;
  const auto tup = unifier.store.make_rvalue_destructured_tuple(std::move(args_copy));
  const auto result_type = unifier.store.make_fresh_type_variable_reference();
  auto lookup = types::Abstraction::clone(source_func, tup, result_type);
  auto lookup_handle = unifier.store.make_abstraction(std::move(lookup));

  const auto sub_lhs_term = make_term(term.source_token, sub.outputs);
  const auto sub_rhs_term = make_term(term.source_token, result_type);
  unifier.substitution->push_type_equation(make_eq(sub_lhs_term, sub_rhs_term));

  const auto arg_lhs_term = make_term(term.source_token, sub.principal_argument);
  const auto arg_rhs_term = make_term(term.source_token, lookup_handle);
  unifier.substitution->push_type_equation(make_eq(arg_lhs_term, arg_rhs_term));

  unifier.check_push_function(lookup_handle, term, MT_ABSTR_REF(*lookup_handle));
  unifier.registered_funcs[source] = true;
}

void SubscriptHandler::maybe_unify_known_subscript_type(Type* source, TermRef term, types::Subscript& sub) {
  using types::Subscript;
  using types::Abstraction;
  using types::DestructuredTuple;

  if (unifier.registered_funcs.count(source) > 0) {
    return;
  }

  assert(!sub.subscripts.empty() && "Expected at least one subscript.");
  const auto& sub0 = sub.subscripts[0];

  if (!unifier.are_concrete_arguments(sub0.arguments)) {
    return;
  }

  TypePtrs into;
  auto args = sub0.arguments;
  args.insert(args.begin(), sub.principal_argument);

  DestructuredTuple tup(DestructuredTuple::Usage::rvalue, std::move(args));

  auto tup_type = unifier.store.make_destructured_tuple(std::move(tup));
  Abstraction abstraction(sub0.method, tup_type, nullptr);

  auto maybe_func = unifier.library.lookup_function(abstraction);
  if (maybe_func) {
//    MT_SHOW1("OK: found subscript signature: ", tup_type);
    const auto func = maybe_func.value();

    if (func->is_abstraction()) {
      const auto lhs_term = make_term(term.source_token, sub.outputs);
      const auto rhs_term = make_term(term.source_token, MT_ABSTR_REF(*func).outputs);
      unifier.substitution->push_type_equation(make_eq(lhs_term, rhs_term));

    } else {
      assert(func->is_scheme());
      const auto& func_scheme = MT_SCHEME_REF(*func);
      const auto func_ref = unifier.instantiation.instantiate(func_scheme);
      assert(func_ref->is_abstraction());

      const auto sub_lhs_term = make_term(term.source_token, sub.outputs);
      const auto sub_rhs_term = make_term(term.source_token, MT_ABSTR_REF(*func_ref).outputs);
      unifier.substitution->push_type_equation(make_eq(sub_lhs_term, sub_rhs_term));

      const auto tup_lhs_term = make_term(term.source_token, tup_type);
      const auto tup_rhs_term = make_term(term.source_token, MT_ABSTR_REF(*func_ref).inputs);
      unifier.substitution->push_type_equation(make_eq(tup_lhs_term, tup_rhs_term));
    }

    if (sub.subscripts.size() > 1) {
      auto result_type = unifier.store.make_fresh_type_variable_reference();

      const auto lhs_term = make_term(term.source_token, result_type);
      const auto rhs_term = make_term(term.source_token, sub.outputs);
      unifier.substitution->push_type_equation(make_eq(lhs_term, rhs_term));

      sub.principal_argument = result_type;
      sub.subscripts.erase(sub.subscripts.begin());
      sub.outputs = unifier.store.make_fresh_type_variable_reference();

    } else {
      unifier.registered_funcs[source] = true;
    }
  } else {
    unifier.add_error(unifier.make_unresolved_function_error(term.source_token, source));
    unifier.registered_funcs[source] = true;
  }
}

void SubscriptHandler::maybe_unify_subscripted_class(Type* source, TermRef term,
                                                     const types::Class& primary_arg, types::Subscript& sub) {
  if (unifier.registered_funcs.count(source) > 0) {
    return;
  }

  assert(!sub.subscripts.empty() && "Expected at least one subscript.");
  const auto& sub0 = sub.subscripts[0];

  if (!unifier.are_concrete_arguments(sub0.arguments) || sub0.method != SubscriptMethod::period) {
    return;
  }

  unifier.registered_funcs[source] = true;

  assert(sub0.arguments.size() == 1 && "Expected 1 argument.");
  assert(primary_arg.source->is_record());
  const auto& record = MT_RECORD_REF(*primary_arg.source);
  const auto& arg0 = sub0.arguments[0];

  if (!arg0->is_constant_value()) {
    unifier.add_error(unifier.make_non_constant_field_reference_expr_error(term.source_token, arg0));
    return;
  }

  const auto& const_val = MT_CONST_VAL_REF(*arg0);
  const auto* matching_field = record.find_field(const_val);

  if (!matching_field) {
    unifier.add_error(unifier.make_reference_to_non_existent_field_error(term.source_token, &primary_arg, arg0));
    return;
  }

#if 0
  const auto prop_type = store.make_list(matching_field->type);
const auto output_type = store.make_output_destructured_tuple(prop_type);
#else
  const auto prop_type = matching_field->type;
  const auto output_type = unifier.store.make_rvalue_destructured_tuple(prop_type);
#endif

  const auto lhs_term = make_term(term.source_token, sub.outputs);
  const auto rhs_term = make_term(term.source_token, output_type);
  unifier.substitution->push_type_equation(make_eq(lhs_term, rhs_term));

  if (sub.subscripts.size() == 1) {
    return;
  }

  auto rest_subs = sub.subscripts;
  rest_subs.erase(rest_subs.begin());
  auto& store = unifier.store;
  auto& substitution = unifier.substitution;

  auto new_sub_ptr = store.make_subscript(output_type, std::move(rest_subs), store.make_fresh_type_variable_reference());
  const auto lhs_sub_term = make_term(term.source_token, store.make_fresh_type_variable_reference());
  const auto rhs_sub_term = make_term(term.source_token, new_sub_ptr);
  substitution->push_type_equation(make_eq(lhs_sub_term, rhs_sub_term));
}

}
