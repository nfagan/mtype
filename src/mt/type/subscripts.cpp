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

SubscriptHandler::SubscriptHandler(mt::Unifier& unifier) :
unifier(unifier), library(unifier.library) {
  //
}

void SubscriptHandler::maybe_unify_subscript(Type* source, TermRef term, types::Subscript& sub) {
  const auto arg0 = sub.principal_argument;

  if (!unifier.is_visited_type(source) && unifier.is_concrete_argument(arg0)) {
    principal_argument_dispatch(source, term, sub, arg0);
  }
}

void SubscriptHandler::principal_argument_dispatch(Type* source, TermRef term,
                                                   types::Subscript& sub,
                                                   Type* arg0) {
  if (arg0->is_alias()) {
    principal_argument_dispatch(source, term, sub, MT_ALIAS_REF(*arg0).source);

  } else if (arg0->is_abstraction()) {
    function_call_subscript(source, term, sub, arg0);

  } else if (arg0->is_scheme() && arg0->scheme_source()->is_abstraction()) {
    scheme_function_call_subscript(source, term, MT_SCHEME_REF(*arg0), sub);

  } else {
    maybe_unify_non_function_subscript(source, term, sub, arg0);
  }
}

void SubscriptHandler::maybe_unify_non_function_subscript(Type* source, TermRef term,
                                                          types::Subscript& sub,
                                                          Type* arg0) {
  assert(!sub.subscripts.empty());
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
    unifier.add_error(make_unresolved_function_error(term.source_token, source));
    return;
  }

  Type* next_arg0 = nullptr;

  if (sub0.is_parens()) {
    if (arg0->is_list()) {
      //  E.g. If `a` == {list<double>}, then `a{1}(1)` is an error.
      unifier.add_error(make_unresolved_function_error(term.source_token, source));
      return;
    } else {
      next_arg0 = arg0;
    }

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

Type* SubscriptHandler::match_members(const Type* source, const Token* source_token,
                                      const TypePtrs& members) {
  if (members.empty()) {
    unifier.add_error(make_unresolved_function_error(source_token, source));
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

Type* SubscriptHandler::tuple_brace_subscript(const Type* source,
                                              const Token* source_token,
                                              const types::Tuple& arg0) {
  return match_members(source, source_token, arg0.members);
}

Type* SubscriptHandler::list_parens_subscript(const Type* source, const Token* source_token,
                                              const types::List& arg0) {
  return match_members(source, source_token, arg0.pattern);
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
    auto err =
      unifier.make_reference_to_non_existent_field_error(source_token, record_source, sub_arg0);
    unifier.add_error(std::move(err));
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

bool SubscriptHandler::are_valid_subscript_arguments(const Type* arg0,
                                                     const types::Subscript::Sub& sub) const {
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
    const auto maybe_lookup =
      types::DestructuredTuple::type_or_first_non_destructured_tuple_member(arg);
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

void SubscriptHandler::scheme_function_call_subscript(Type* source, TermRef term,
                                                      const types::Scheme& scheme,
                                                      types::Subscript& sub) {
  if (unifier.is_visited_type(source)) {
    return;

  } else if (sub.subscripts.size() != 1 || !sub.subscripts[0].is_parens()) {
    unifier.register_visited_type(source);
    auto err =
      unifier.make_invalid_function_invocation_error(term.source_token, sub.principal_argument);
    unifier.add_error(std::move(err));
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
                                               const types::Subscript& sub,
                                               Type* arg0) {
  if (unifier.is_visited_type(source)) {
    return;

  } else if (sub.subscripts.size() != 1 || !sub.subscripts[0].is_parens()) {
    unifier.register_visited_type(source);
    auto err =
      unifier.make_invalid_function_invocation_error(term.source_token, arg0);
    unifier.add_error(std::move(err));
    return;
  }

  unifier.register_visited_type(source);

  const auto& sub0 = sub.subscripts[0];
  auto& store = unifier.store;

  auto args_copy = sub0.arguments;
  const auto args = store.make_rvalue_destructured_tuple(std::move(args_copy));
  auto app = store.make_application(arg0, args, sub.outputs);

  const auto app_lhs_term =
    make_term(term.source_token, store.make_fresh_type_variable_reference());
  const auto app_rhs_term = make_term(term.source_token, app);
  unifier.substitution->push_type_equation(make_eq(app_lhs_term, app_rhs_term));
}

}
