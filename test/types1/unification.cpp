#include "unification.hpp"
#include "typing.hpp"
#include "debug.hpp"
#include <algorithm>

#define MT_SHOW1(msg, a) \
  std::cout << (msg); \
  type_printer().show((a)); \
  std::cout << std::endl

#define MT_SHOW2(msg, a, b) \
  std::cout << (msg) << std::endl; \
  type_printer().show2((a), (b)); \
  std::cout << std::endl

namespace mt {

bool Unifier::is_known_subscript_type(const TypeHandle& handle) const {
  return is_concrete_argument(handle) && types_with_known_subscripts.count(handle) > 0;
}

void Unifier::flatten_destructured_tuple(const types::DestructuredTuple& source, std::vector<TypeHandle>& into) const {
  for (const auto& mem : source.members) {
    if (type_of(mem) == Type::Tag::destructured_tuple) {
      flatten_destructured_tuple(store.at(mem).destructured_tuple, into);
    } else {
      into.push_back(mem);
    }
  }
}

bool Unifier::is_concrete_argument(const TypeHandle& handle) const {
  using Tag = Type::Tag;
  const auto& type = store.at(handle);

  switch (type.tag) {
    case Tag::destructured_tuple:
      return is_concrete_argument(type.destructured_tuple);
    case Tag::list:
      return is_concrete_argument(type.list);
    case Tag::tuple:
    case Tag::scalar:
      return true;
    default:
      return false;
  }
}

bool Unifier::are_concrete_arguments(const std::vector<TypeHandle>& args) const {
  for (const auto& arg : args) {
    if (!is_concrete_argument(arg)) {
      return false;
    }
  }
  return true;
}

bool Unifier::is_concrete_argument(const types::DestructuredTuple& tup) const {
  return are_concrete_arguments(tup.members);
}

bool Unifier::is_concrete_argument(const types::List& list) const {
  return are_concrete_arguments(list.pattern);
}

void Unifier::check_push_func(const TypeHandle& source, const types::Abstraction& func) {
  if (registered_funcs.count(source) > 0) {
    return;
  }

  assert(type_of(func.inputs) == Type::Tag::destructured_tuple);
  const auto& inputs = store.at(func.inputs).destructured_tuple;

  if (!is_concrete_argument(inputs)) {
    return;
  }

  auto func_it = function_types.find(func);
  if (func_it != function_types.end()) {
    const auto& func_ref = store.at(func_it->second);

    if (func_ref.is_abstraction()) {
      push_type_equation(TypeEquation(source, func_it->second));

    } else {
      assert(func_ref.is_scheme());
      const auto& func_scheme = func_ref.scheme;
      const auto new_abstr_handle = instantiation.instantiate(func_scheme);
      assert(type_of(new_abstr_handle) == Type::Tag::abstraction);
      push_type_equation(TypeEquation(source, new_abstr_handle));
    }

  } else {
    std::cout << "ERROR: no such function: ";
    type_printer().show(source);
    std::cout << std::endl;
  }

  registered_funcs[source] = true;
}

void Unifier::check_assignment(const TypeHandle& source, const types::Assignment& assignment) {
  if (registered_assignments.count(source) > 0) {
    return;
  }

  if (is_concrete_argument(assignment.rhs)) {
    push_type_equation(TypeEquation(assignment.lhs, assignment.rhs));
    registered_assignments[source] = true;
  }
}

Optional<TypeHandle> Unifier::bound_type(const TypeHandle& for_type) const {
  if (bound_variables.count(for_type) == 0) {
    return NullOpt{};
  } else {
    return Optional<TypeHandle>(bound_variables.at(for_type));
  }
}

void Unifier::unify() {
  make_known_types();

  int64_t i = 0;
  while (i < type_equations.size()) {
    unify_one(type_equations[i++]);
  }

  show();
}

TypeHandle Unifier::apply_to(const TypeHandle& source, mt::types::Variable& var) {
  if (bound_variables.count(source) > 0) {
    return bound_variables.at(source);
  } else {
    return source;
  }
}

TypeHandle Unifier::apply_to(const TypeHandle& source, types::Tuple& tup) {
  apply_to(tup.members);
  return source;
}

TypeHandle Unifier::apply_to(const TypeHandle& source, types::DestructuredTuple& tup) {
  apply_to(tup.members);
  return source;
}

TypeHandle Unifier::apply_to(const TypeHandle& source, types::Abstraction& func) {
  func.inputs = apply_to(func.inputs);
  func.outputs = apply_to(func.outputs);

  check_push_func(source, func);

  return source;
}

TypeHandle Unifier::apply_to(const TypeHandle& source, types::Subscript& sub) {
  sub.principal_argument = apply_to(sub.principal_argument);
  for (auto& s : sub.subscripts) {
    apply_to(s.arguments);
  }
  sub.outputs = apply_to(sub.outputs);
  return source;
}

TypeHandle Unifier::apply_to(const TypeHandle& source, types::List& list) {
  apply_to(list.pattern);
  return source;
}

TypeHandle Unifier::apply_to(const TypeHandle& source, types::Assignment& assignment) {
  assignment.lhs = apply_to(assignment.lhs);
  assignment.rhs = apply_to(assignment.rhs);

  check_assignment(source, assignment);

  return source;
}

TypeHandle Unifier::apply_to(const TypeHandle& source) {
  auto& type = store.at(source);

  switch (type.tag) {
    case Type::Tag::variable:
      return apply_to(source, type.variable);
    case Type::Tag::scalar:
      return source;
    case Type::Tag::abstraction:
      return apply_to(source, type.abstraction);
    case Type::Tag::tuple:
      return apply_to(source, type.tuple);
    case Type::Tag::destructured_tuple:
      return apply_to(source, type.destructured_tuple);
    case Type::Tag::subscript:
      return apply_to(source, type.subscript);
    case Type::Tag::list:
      return apply_to(source, type.list);
    case Type::Tag::assignment:
      return apply_to(source, type.assignment);
    default:
      MT_SHOW1("Unhandled apply to: ", source);
      assert(false);
      return source;
  }
}

void Unifier::apply_to(std::vector<TypeHandle>& sources) {
  for (auto& source : sources) {
    source = apply_to(source);
  }
}

TypeHandle Unifier::substitute_one(const TypeHandle& source, const TypeHandle& lhs, const TypeHandle& rhs) {
  auto& type = store.at(source);

  switch (type.tag) {
    case Type::Tag::variable:
      return substitute_one(type.variable, source, lhs, rhs);
    case Type::Tag::scalar:
      return source;
    case Type::Tag::abstraction:
      return substitute_one(type.abstraction, source, lhs, rhs);
    case Type::Tag::tuple:
      return substitute_one(type.tuple, source, lhs, rhs);
    case Type::Tag::destructured_tuple:
      return substitute_one(type.destructured_tuple, source, lhs, rhs);
    case Type::Tag::subscript:
      return substitute_one(type.subscript, source, lhs, rhs);
    case Type::Tag::list:
      return substitute_one(type.list, source, lhs, rhs);
    case Type::Tag::assignment:
      return substitute_one(type.assignment, source, lhs, rhs);
    default:
      MT_SHOW1("Unhandled: ", source);
      assert(false);
      return source;
  }
}

void Unifier::substitute_one(std::vector<TypeHandle>& sources, const TypeHandle& lhs, const TypeHandle& rhs) {
  for (auto& source : sources) {
    source = substitute_one(source, lhs, rhs);
  }
}

TypeHandle Unifier::substitute_one(types::Assignment& assignment, const TypeHandle& source,
                                     const TypeHandle& lhs, const TypeHandle& rhs) {
  assignment.rhs = substitute_one(assignment.rhs, lhs, rhs);
  assignment.lhs = substitute_one(assignment.lhs, lhs, rhs);

  check_assignment(source, assignment);

  return source;
}

TypeHandle Unifier::substitute_one(types::Abstraction& func, const TypeHandle& source,
                                   const TypeHandle& lhs, const TypeHandle& rhs) {
  func.inputs = substitute_one(func.inputs, lhs, rhs);
  func.outputs = substitute_one(func.outputs, lhs, rhs);

  check_push_func(source, func);

  return source;
}

TypeHandle Unifier::substitute_one(types::Tuple& tup, const TypeHandle& source,
                                   const TypeHandle& lhs, const TypeHandle& rhs) {
  substitute_one(tup.members, lhs, rhs);
  return source;
}

TypeHandle Unifier::substitute_one(types::DestructuredTuple& tup, const TypeHandle& source,
                                   const TypeHandle& lhs, const TypeHandle& rhs) {
  substitute_one(tup.members, lhs, rhs);
  return source;
}

TypeHandle Unifier::substitute_one(types::Variable& var, const TypeHandle& source,
                                   const TypeHandle& lhs, const TypeHandle& rhs) {
  if (source == lhs) {
    return rhs;
  } else {
    return source;
  }
}

TypeHandle Unifier::substitute_one(types::Subscript& sub, const TypeHandle& source,
                                   const TypeHandle& lhs, const TypeHandle& rhs) {
  sub.principal_argument = substitute_one(sub.principal_argument, lhs, rhs);

  for (auto& s : sub.subscripts) {
    substitute_one(s.arguments, lhs, rhs);
  }

  sub.outputs = substitute_one(sub.outputs, lhs, rhs);
  maybe_unify_subscript(source, sub);
  return source;
}

void Unifier::flatten_list(const TypeHandle& source, std::vector<TypeHandle>& into) const {
  const auto& mem = store.at(source);

  switch (mem.tag) {
    case Type::Tag::list: {
      auto sz = mem.list.size();
      for (int64_t i = 0; i < sz; i++) {
        flatten_list(mem.list.pattern[i], into);
      }
      break;
    }
    case Type::Tag::destructured_tuple: {
      const auto& tup = mem.destructured_tuple;
      const auto sz = tup.is_outputs() ? std::min(int64_t(1), tup.size()) : tup.size();
      for (int64_t i = 0; i < sz; i++) {
        flatten_list(tup.members[i], into);
      }
      break;
    }
    default:
      into.push_back(source);
  }
}

TypeHandle Unifier::substitute_one(types::List& list, const TypeHandle& source,
                                   const TypeHandle& lhs, const TypeHandle& rhs) {
  std::vector<TypeHandle> flattened;
  flatten_list(source, flattened);
  std::swap(list.pattern, flattened);

  TypeHandle last;
  int64_t remove_from = 1;
  int64_t num_remove = 0;

  for (int64_t i = 0; i < list.size(); i++) {
    auto& element = list.pattern[i];
    element = substitute_one(element, lhs, rhs);
    assert(element.is_valid());
    if (element == last) {
      num_remove++;
    } else {
      num_remove = 0;
      remove_from = i + 1;
    }
    last = element;
  }

  if (num_remove > 0) {
    list.pattern.erase(list.pattern.begin() + remove_from, list.pattern.end());
  }

  return source;
}

void Unifier::unify_one(TypeEquation eq) {
  using Tag = Type::Tag;

  eq.lhs = apply_to(eq.lhs);
  eq.rhs = apply_to(eq.rhs);

  if (eq.lhs == eq.rhs) {
    return;
  }

  const auto lhs_type = type_of(eq.lhs);
  const auto rhs_type = type_of(eq.rhs);

  if (lhs_type != Tag::variable && rhs_type != Tag::variable) {
    bool success = simplify(eq.lhs, eq.rhs);

    if (!success) {
      MT_SHOW2("Failed to simplify: ", eq.lhs, eq.rhs);
      assert(false);
    }

    return;

  } else if (lhs_type != Tag::variable) {
    assert(rhs_type == Tag::variable && "Rhs should be variable.");
    std::swap(eq.lhs, eq.rhs);
  }

  for (auto& subst_it : bound_variables) {
    subst_it.second = substitute_one(subst_it.second, eq.lhs, eq.rhs);
  }

  bound_variables[eq.lhs] = eq.rhs;
}

void Unifier::push_type_equations(const std::vector<TypeHandle>& t0, const std::vector<TypeHandle>& t1, int64_t num) {
  assert(num >= 0 && num <= t0.size() && num <= t1.size() && "out of bounds read.");
  for (int64_t i = 0; i < num; i++) {
    push_type_equation(TypeEquation(t0[i], t1[i]));
  }
}

bool Unifier::simplify_same_types(const TypeHandle& lhs, const TypeHandle& rhs) {
  using Tag = Type::Tag;

  const auto& t0 = store.at(lhs);
  const auto& t1 = store.at(rhs);

  switch (t0.tag) {
    case Tag::abstraction:
      return simplify(t0.abstraction, t1.abstraction);
    case Tag::scalar:
      return simplify(t0.scalar, t1.scalar);
    case Tag::tuple:
      return simplify(t0.tuple, t1.tuple);
    case Tag::destructured_tuple:
      return simplify(t0.destructured_tuple, t1.destructured_tuple);
    case Tag::list:
      return simplify(t0.list, t1.list);
    case Tag::variable: {
      push_type_equation(TypeEquation(lhs, rhs));
      return true;
    }
    default:
      MT_SHOW2("Unhandled same types for simplify: ", lhs, rhs);
      assert(false);
      return false;
  }
}

bool Unifier::simplify_different_types(const TypeHandle& lhs, const TypeHandle& rhs) {
  using Tag = Type::Tag;

  const auto lhs_type = type_of(lhs);
  const auto rhs_type = type_of(rhs);

  if (lhs_type == Tag::variable) {
    push_type_equation(TypeEquation(lhs, rhs));
    return true;

  } else if (rhs_type == Tag::variable) {
    push_type_equation(TypeEquation(rhs, lhs));
    return true;

  } else if (lhs_type == Tag::destructured_tuple) {
    return simplify_different_types(store.at(lhs).destructured_tuple, lhs, rhs);

  } else if (rhs_type == Tag::destructured_tuple) {
    return simplify_different_types(store.at(rhs).destructured_tuple, rhs, lhs);

  } else if (lhs_type == Tag::list) {
    return simplify_different_types(store.at(lhs).list, lhs, rhs);

  } else if (rhs_type == Tag::list) {
    return simplify_different_types(store.at(rhs).list, rhs, lhs);

  } else {
    MT_SHOW2("Unhandled simplify for diff types: ", lhs, rhs);
    assert(false);
    return false;
  }
}

bool Unifier::simplify(const TypeHandle& lhs, const TypeHandle& rhs) {
  if (type_of(lhs) == type_of(rhs)) {
    return simplify_same_types(lhs, rhs);
  } else {
    return simplify_different_types(lhs, rhs);
  }
}

bool Unifier::simplify_different_types(const types::List& list, const TypeHandle& source, const TypeHandle& rhs) {
  const auto& rhs_member = store.at(rhs);
  if (rhs_member.is_scalar()) {
    for (const auto& el : list.pattern) {
      push_type_equation(TypeEquation(el, rhs));
    }
    return true;
  }

  MT_SHOW2("Cannot simplify list with non-scalar type.", source, rhs);
  assert(false);

  return false;
}

bool Unifier::simplify_different_types(const types::DestructuredTuple& tup, const TypeHandle& source, const TypeHandle& rhs) {
  using Use = types::DestructuredTuple::Usage;

  auto t = store.make_destructured_tuple(Use::rvalue, rhs);
  push_type_equation(TypeEquation(source, t));

  return true;
}

bool Unifier::simplify(const types::Scalar& t0, const types::Scalar& t1) {
  return t0.identifier == t1.identifier;
}

bool Unifier::simplify(const types::Abstraction& t0, const types::Abstraction& t1) {
  return simplify(t0.inputs, t1.inputs) && simplify(t0.outputs, t1.outputs);
}

bool Unifier::simplify_expanding_members(const types::DestructuredTuple& a,
                                         const types::DestructuredTuple& b) {
  const int64_t num_a = a.members.size();
  const int64_t num_b = b.members.size();

  int64_t ia = 0;
  int64_t ib = 0;

  while (ia < num_a && ib < num_b) {
    bool res = simplify_recurse_tuple(a, b, &ia, &ib);
    if (!res) {
      return false;
    }
  }

  if (ia == num_a && ib == num_b) {
    return true;

  } else if (a.is_outputs() && b.is_value_usage()) {
    return ib == num_b && ia == num_b;

  } else if (b.is_outputs() && a.is_value_usage()) {
    return ia == num_a && ib == num_a;

  } else {
    return false;
  }
}

bool Unifier::simplify_recurse_tuple(const types::DestructuredTuple& a,
                                     const types::DestructuredTuple& b,
                                     int64_t* ia, int64_t* ib) {
  assert(*ia < a.size() && *ib < b.size());

  const auto& mem_a = a.members[*ia];
  const auto& mem_b = b.members[*ib];

  const auto& va = store.at(mem_a);
  const auto& vb = store.at(mem_b);

  if (va.is_list() && !vb.is_list()) {
    if (b.is_definition_usage()) {
      return false;
    }

    bool success = match_list(va.list, b, ib);
    (*ia)++;
    return success;

  } else if (vb.is_list() && !va.is_list()) {
    if (a.is_definition_usage()) {
      return false;
    }

    bool success = match_list(vb.list, a, ia);
    (*ib)++;
    return success;

  } else if (va.is_destructured_tuple()) {
    return simplify_subrecurse_tuple(a, b, va.destructured_tuple, ia, ib);

  } else if (vb.is_destructured_tuple()) {
    return simplify_subrecurse_tuple(b, a, vb.destructured_tuple, ib, ia);

  } else {
    (*ia)++;
    (*ib)++;

    return simplify(mem_a, mem_b);
  }
}

bool Unifier::simplify_subrecurse_tuple(const types::DestructuredTuple& a,
                                        const types::DestructuredTuple& b,
                                        const types::DestructuredTuple& sub_a,
                                        int64_t* ia, int64_t* ib) {
  int64_t ia_child = 0;
  const int64_t num_sub_a = sub_a.size();
  int64_t expect_match = num_sub_a;

  if (a.is_value_usage() && sub_a.is_outputs()) {
    expect_match = 1;
  }

  bool success = true;

  while (success && ia_child < expect_match && ia_child < num_sub_a && *ib < b.size()) {
    success = simplify_recurse_tuple(sub_a, b, &ia_child, ib);
  }

  (*ia)++;

  return success && ia_child == expect_match;
}

bool Unifier::simplify_subrecurse_list(const types::List& a, int64_t* ia,
                                       const types::DestructuredTuple& b, const TypeHandle& mem_b) {

  const auto& mem_a = a.pattern[*ia];

  if (type_of(mem_b) == Type::Tag::destructured_tuple) {
    const auto& sub_b = store.at(mem_b).destructured_tuple;
    const int64_t num_b = sub_b.size();

    int64_t expect_num_b = num_b;
    if (b.is_value_usage() && sub_b.is_outputs()) {
      expect_num_b = 1;
    }

    int64_t ib = 0;
    bool success = true;

    while (success && ib < expect_num_b && ib < num_b) {
      success = simplify_subrecurse_list(a, ia, sub_b, sub_b.members[ib++]);
    }

    return success && ib == expect_num_b;

  } else {
    *ia = (*ia + 1) % a.size();
    return simplify(mem_a, mem_b);
  }
}

bool Unifier::match_list(const types::List& a, const types::DestructuredTuple& b, int64_t* ib) {
  int64_t ia = 0;
  bool success = true;

  while (success && ia < a.size() && *ib < b.size()) {
    success = simplify_subrecurse_list(a, &ia, b, b.members[(*ib)++]);
  }

  return success && (a.size() == 0 || (ia == 0 && *ib == b.size()));
}

bool Unifier::simplify(const types::DestructuredTuple& t0, const types::DestructuredTuple& t1) {
  if (types::DestructuredTuple::mismatching_definition_usages(t0, t1)) {
    return false;

  } else if (t0.is_definition_usage() && t0.usage == t1.usage) {
    return simplify(t0.members, t1.members);

  } else {
    return simplify_expanding_members(t0, t1);
  }
}

bool Unifier::simplify(const types::List& t0, const types::List& t1) {
  const int64_t num_a = t0.size();
  const int64_t num_b = t1.size();
  const int64_t sz = std::max(num_a, num_b);

  if ((num_a == 0 || num_b == 0) && sz > 0) {
    //  Empty list with non-empty list.
    return false;
  }

  for (int64_t i = 0; i < sz; i++) {
    const auto& mem_a = t0.pattern[i % num_a];
    const auto& mem_b = t1.pattern[i % num_b];

    push_type_equation(TypeEquation(mem_a, mem_b));
  }

  return true;
}

bool Unifier::simplify(const types::Tuple& t0, const types::Tuple& t1) {
  return simplify(t0.members, t1.members);
}

bool Unifier::simplify(const std::vector<TypeHandle>& t0, const std::vector<TypeHandle>& t1) {
  const auto sz0 = t0.size();
  if (sz0 != t1.size()) {
    return false;
  }
  push_type_equations(t0, t1, sz0);
  return true;
}

void Unifier::maybe_unify_subscript(const TypeHandle& source, types::Subscript& sub) {
  if (is_known_subscript_type(sub.principal_argument)) {
    maybe_unify_known_subscript_type(source, sub);

  } else if (type_of(sub.principal_argument) != Type::Tag::variable) {
    MT_SHOW1("Tried to unify subscript with principal arg: ", sub.principal_argument);
    assert(false);
  }
}

bool Unifier::maybe_unify_known_subscript_type(const TypeHandle& source, types::Subscript& sub) {
  using types::Subscript;
  using types::Abstraction;
  using types::DestructuredTuple;

  if (registered_funcs.count(source) > 0) {
    return true;
  }

  assert(!sub.subscripts.empty() && "Expected at least one subscript.");
  const auto& sub0 = sub.subscripts[0];
  assert(!sub0.arguments.empty());

  if (!are_concrete_arguments(sub0.arguments)) {
    return true;
  }

  std::vector<TypeHandle> into;
  auto args = sub0.arguments;
  args.insert(args.begin(), sub.principal_argument);

  DestructuredTuple tup(DestructuredTuple::Usage::rvalue, std::move(args));

  auto tup_type = store.make_type();
  store.assign(tup_type, Type(std::move(tup)));
  Abstraction abstraction(sub0.method, tup_type, TypeHandle());

  auto func_it = function_types.find(abstraction);
  if (func_it != function_types.end()) {
//    MT_SHOW1("OK: found subscript signature: ", tup_type);
    const auto& func = store.at(func_it->second);

    if (func.is_abstraction()) {
      push_type_equation(TypeEquation(sub.outputs, func.abstraction.outputs));

    } else {
      assert(func.tag == Type::Tag::scheme);
      const auto& func_scheme = func.scheme;
      const auto new_abstr_handle = instantiation.instantiate(func_scheme);
      const auto& func_ref = store.at(new_abstr_handle);
      assert(func_ref.is_abstraction());

      push_type_equation(TypeEquation(sub.outputs, func_ref.abstraction.outputs));
      push_type_equation(TypeEquation(tup_type, func_ref.abstraction.inputs));
    }

    if (sub.subscripts.size() > 1) {
      auto result_type = store.make_fresh_type_variable_reference();
      push_type_equation(TypeEquation(result_type, sub.outputs));

      sub.principal_argument = result_type;
      sub.subscripts.erase(sub.subscripts.begin());
      sub.outputs = store.make_fresh_type_variable_reference();

    } else {
      registered_funcs[source] = true;
    }

    return true;

  } else {
    MT_SHOW1("ERROR: no such subscript signature: ", tup_type);
    registered_funcs[source] = true;

    return false;
  }
}

Type::Tag Unifier::type_of(const mt::TypeHandle& handle) const {
  return store.at(handle).tag;
}

DebugTypePrinter Unifier::type_printer() const {
  return DebugTypePrinter(store, string_registry);
}

void Unifier::show() {
  std::cout << "----" << std::endl;
  for (const auto& t : store.get_types()) {
    if (t.tag == Type::Tag::abstraction) {
      type_printer().show(t.abstraction);
      std::cout << std::endl;
    }
  }
}

void Unifier::make_known_types() {
  make_subscript_references();
  make_binary_operators();
  make_free_functions();
  make_concatenations();
}

void Unifier::make_free_functions() {
  make_min();
  make_fileparts();
  make_list_outputs_type();
  make_list_inputs_type();
  make_list_outputs_type2();
}

void Unifier::make_min() {
  using types::DestructuredTuple;
  using types::Abstraction;

  const auto args_type = store.make_type();
  const auto result_type = store.make_type();
  const auto func_type = store.make_type();

  const auto& double_handle = store.double_type_handle;
  const auto& char_handle = store.char_type_handle;
  const auto& str_handle = store.string_type_handle;
  const auto name = std::string("min");
  const auto ident = MatlabIdentifier(string_registry.register_string(name));

  Abstraction func(ident, args_type, result_type);
  auto func_copy = func;

  store.assign(args_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_inputs, double_handle, str_handle)));
  store.assign(result_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_outputs, double_handle, char_handle)));

  store.assign(func_type, Type(std::move(func)));

  function_types[func_copy] = func_type;
}

void Unifier::make_fileparts() {
  using types::DestructuredTuple;
  using types::Abstraction;

  const auto args_type = store.make_type();
  const auto result_type = store.make_type();
  const auto func_type = store.make_type();

  const auto& double_handle = store.double_type_handle;
  const auto& char_handle = store.char_type_handle;

  const auto name = std::string("fileparts");
  const auto ident = MatlabIdentifier(string_registry.register_string(name));

  Abstraction func(ident, args_type, result_type);
  auto func_copy = func;

  std::vector<TypeHandle> outputs{char_handle, double_handle, double_handle};

  store.assign(args_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_inputs, char_handle, double_handle)));
  store.assign(result_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_outputs, std::move(outputs))));

  store.assign(func_type, Type(std::move(func)));

  function_types[func_copy] = func_type;
}

void Unifier::make_list_inputs_type() {
  using types::DestructuredTuple;
  using types::Abstraction;
  using types::List;

  const auto args_type = store.make_type();
  const auto result_type = store.make_type();
  const auto func_type = store.make_type();
  const auto& double_handle = store.double_type_handle;
  const auto list_type = store.make_list(double_handle);

  const auto ident = MatlabIdentifier(string_registry.register_string("in_list"));

  Abstraction func(ident, args_type, result_type);
  auto func_copy = func;

  store.assign(args_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_inputs, TypeHandles{double_handle, list_type})));
  store.assign(result_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_outputs, double_handle)));
  store.assign(func_type, Type(std::move(func)));

  function_types[func_copy] = func_type;
}

void Unifier::make_list_outputs_type() {
  using types::DestructuredTuple;
  using types::Abstraction;
  using types::List;

  const auto args_type = store.make_type();
  const auto result_type = store.make_type();
  const auto func_type = store.make_type();
  const auto list_type = store.make_type();

  const auto& double_handle = store.double_type_handle;
  const auto& char_handle = store.char_type_handle;
  const auto& string_handle = store.string_type_handle;

  const auto name = std::string("lists");
  const auto ident = MatlabIdentifier(string_registry.register_string(name));

  Abstraction func(ident, args_type, result_type);
  auto func_copy = func;

  std::vector<TypeHandle> outputs{double_handle, list_type};
  std::vector<TypeHandle> list_pattern{char_handle, double_handle};

  store.assign(list_type, Type(List(std::move(list_pattern))));
  store.assign(args_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_inputs, double_handle)));
  store.assign(result_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_outputs, std::move(outputs))));
  store.assign(func_type, Type(std::move(func)));

  function_types[func_copy] = func_type;
}

void Unifier::make_list_outputs_type2() {
  using types::DestructuredTuple;
  using types::Abstraction;
  using types::List;

  const auto args_type = store.make_type();
  const auto result_type = store.make_type();
  const auto func_type = store.make_type();

  const auto& double_handle = store.double_type_handle;
  const auto& char_handle = store.char_type_handle;
  const auto& string_handle = store.string_type_handle;

  const auto name = std::string("out_list");
  const auto ident = MatlabIdentifier(string_registry.register_string(name));

  Abstraction func(ident, args_type, result_type);
  auto func_copy = func;

  auto list_type = store.make_list(double_handle);

  store.assign(args_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_inputs, double_handle)));
  store.assign(result_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_outputs, list_type)));
  store.assign(func_type, Type(std::move(func)));

  function_types[func_copy] = func_type;
}

void Unifier::make_subscript_references() {
  make_builtin_parens_subscript_references();
  make_builtin_brace_subscript_reference();
}

void Unifier::make_builtin_parens_subscript_references() {
  using types::DestructuredTuple;
  using types::Abstraction;
  using types::List;

  std::vector<TypeHandle> default_array_types{store.double_type_handle, store.char_type_handle};

  for (const auto& referent_type_handle : default_array_types) {
    const auto args_type = store.make_type();
    const auto list_type = store.make_type();
    const auto result_type = store.make_type();
    const auto func_type = store.make_type();

    Abstraction ref(SubscriptMethod::parens, args_type, result_type);
    auto ref_copy = ref;

    std::vector<TypeHandle> list_types{store.double_type_handle};

    //  T(list<double>) -> T
    store.assign(list_type, Type(List(std::move(list_types))));
    store.assign(args_type,
      Type(DestructuredTuple(DestructuredTuple::Usage::definition_inputs, referent_type_handle, list_type)));
    store.assign(result_type,
      Type(DestructuredTuple(DestructuredTuple::Usage::definition_outputs, referent_type_handle)));

    store.assign(func_type, Type(std::move(ref)));

    function_types[ref_copy] = func_type;
    types_with_known_subscripts.insert(referent_type_handle);
  }
}

void Unifier::make_builtin_brace_subscript_reference() {
  using types::DestructuredTuple;
  using types::Abstraction;
  using types::List;

  auto func_scheme = store.make_type();
  auto ref_var = store.make_fresh_type_variable_reference();
  auto tup_type = store.make_type();

  types::Scheme scheme;

  //  {T}
  store.assign(tup_type, Type(types::Tuple(ref_var)));

  const auto args_type = store.make_type();
  const auto list_subs_type = store.make_type();
  const auto list_result_type = store.make_type();
  const auto result_type = store.make_type();
  const auto func_type = store.make_type();

  types::Abstraction ref(SubscriptMethod::brace, args_type, result_type);
  auto ref_copy = ref;

  store.assign(list_subs_type, Type(List(store.double_type_handle)));  //  list<double>
  store.assign(list_result_type, Type(List(ref_var)));  //  list<T>

  store.assign(args_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_inputs, tup_type, list_subs_type)));
  store.assign(result_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_outputs, list_result_type)));

  scheme.type = func_type;
  scheme.parameters.push_back(ref_var);

  store.assign(func_type, Type(std::move(ref)));
  store.assign(func_scheme, Type(std::move(scheme)));

  function_types[ref_copy] = func_scheme;
  types_with_known_subscripts.insert(tup_type);
}

void Unifier::make_concatenations() {
  using types::DestructuredTuple;
  using types::Abstraction;
  using types::List;

  auto func_scheme = store.make_type();
  auto tvar = store.make_fresh_type_variable_reference(); //  T

  types::Scheme scheme;

  const auto args_type = store.make_type();
  const auto result_type = store.make_type();
  const auto func_type = store.make_type();

  types::Abstraction cat(ConcatenationDirection::horizontal, args_type, result_type);
  auto cat_copy = cat;

  auto list_args_type = store.make_list(tvar);  //  list<T>

  store.assign(args_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_inputs, list_args_type)));
  store.assign(result_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_outputs, tvar)));

  scheme.type = func_type;
  scheme.parameters.push_back(tvar);

  store.assign(func_type, Type(std::move(cat)));
  store.assign(func_scheme, Type(std::move(scheme)));

  function_types[cat_copy] = func_scheme;
}

void Unifier::make_binary_operators() {
  using types::DestructuredTuple;
  using types::Abstraction;

  std::vector<BinaryOperator> operators{BinaryOperator::plus, BinaryOperator::minus,
                                        BinaryOperator::times, BinaryOperator::matrix_times,
                                        BinaryOperator::right_divide};

  for (const auto& op : operators) {
    auto args_type = store.make_type();
    auto result_type = store.make_type();
    auto func_type = store.make_type();

    Abstraction abstr(op, args_type, result_type);
    Abstraction abstr_copy = abstr;

    const auto& double_handle = store.double_type_handle;

    store.assign(args_type,
      Type(DestructuredTuple(DestructuredTuple::Usage::definition_inputs, double_handle, double_handle)));
    store.assign(result_type,
      Type(DestructuredTuple(DestructuredTuple::Usage::definition_outputs, double_handle)));
    store.assign(func_type, Type(std::move(abstr)));

    function_types[abstr_copy] = func_type;
  }
}

}
