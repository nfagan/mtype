#include "unification.hpp"
#include "typing.hpp"
#include "debug.hpp"

namespace mt {

bool Unifier::is_recursive_destructured_tuple(const TypeHandle& handle) const {
  if (type_of(handle) != Type::Tag::destructured_tuple) {
    return false;
  }
  const auto& tup = store.at(handle).destructured_tuple;
  for (const auto& mem : tup.members) {
    if (type_of(mem) == Type::Tag::destructured_tuple) {
      return true;
    }
  }
  return false;
}

bool Unifier::is_known_subscript_type(const TypeHandle& handle) const {
  return types_with_known_subscripts.count(handle) > 0;
}

bool Unifier::is_structured_tuple_type(const mt::TypeHandle& handle) const {
  assert(handle.is_valid());
  const auto& t = store.at(handle);
  return t.tag == Type::Tag::tuple;
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
    push_type_equation(TypeEquation(source, func_it->second));

  } else {
    std::cout << "ERROR: no such function: ";
    type_printer().show(source);
    std::cout << std::endl;
  }

  registered_funcs[source] = true;
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
    default:
      assert(false && "Unhandled.");
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
    default:
      assert(false);
  }
}

void Unifier::substitute_one(std::vector<TypeHandle>& sources, const TypeHandle& lhs, const TypeHandle& rhs) {
  for (auto& source : sources) {
    source = substitute_one(source, lhs, rhs);
  }
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

TypeHandle Unifier::substitute_one(types::List& list, const TypeHandle& source,
                                   const TypeHandle& lhs, const TypeHandle& rhs) {
  TypeHandle last;
  int64_t remove_from = 1;
  int64_t num_remove = 0;

  for (int64_t i = 0; i < list.pattern.size(); i++) {
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
      type_printer().show2(eq.lhs, eq.rhs);
      std::cout << std::endl;
    }

    assert(success && "Failed to simplify.");
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
    default:
      std::cout << "Unhandled same types for simplify: " << std::endl;
      type_printer().show2(lhs, rhs);
      std::cout << std::endl;
      assert(false);
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

  } else {
    assert(false);
    std::cout << "Cannot simplify: " << type_of(lhs) << ", " << type_of(rhs) << std::endl;
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

bool Unifier::simplify_different_types(const types::DestructuredTuple& tup, const TypeHandle& source, const TypeHandle& rhs) {
  using Use = types::DestructuredTuple::Usage;

  auto t = store.make_type();
  store.assign(t, Type(types::DestructuredTuple(Use::rvalue, rhs)));
  push_type_equation(TypeEquation(source, t));

  return true;
}

bool Unifier::simplify(const types::Scalar& t0, const types::Scalar& t1) {
  return t0.identifier.name == t1.identifier.name;
}

bool Unifier::simplify(const types::Abstraction& t0, const types::Abstraction& t1) {
  return simplify(t0.inputs, t1.inputs) && simplify(t0.outputs, t1.outputs);
}

bool Unifier::simplify_match_arguments(const types::DestructuredTuple& t0, const types::DestructuredTuple& t1) {
  return simplify(t0.members, t1.members);
}

bool Unifier::simplify_different_usage(const types::DestructuredTuple& a, const types::DestructuredTuple& b) {
  using Tag = Type::Tag;
  using Use = types::DestructuredTuple::Usage;

  assert(!types::DestructuredTuple::mismatching_definition_usages(a, b));

  const int64_t num_a = a.members.size();
  const int64_t num_b = b.members.size();

  int64_t ia = 0;
  int64_t ib = 0;

  while (ia < num_a && ib < num_b) {
    const auto& mem_a = a.members[ia];
    const auto& mem_b = b.members[ib];

    const auto& va = store.at(mem_a);
    const auto& vb = store.at(mem_b);

    const auto ta = va.tag;
    const auto tb = vb.tag;

    bool res;
    int64_t num_incr_a = 1;
    int64_t num_incr_b = 1;

    if (ta == tb) {
      if (ta == Type::Tag::variable) {
        push_type_equation(TypeEquation(mem_a, mem_b));
        res = true;
      } else {
        res = simplify_same_types(mem_a, mem_b);
      }

    } else if (va.is_list()) {
      res = simplify_match_list(mem_a, b.members, ib, &num_incr_b);

    } else if (vb.is_list()) {
      res = simplify_match_list(mem_b, a.members, ia, &num_incr_a);

    } else {
      res = simplify(mem_a, mem_b);
    }

    if (!res) {
      type_printer().show2(mem_a, mem_b);
      std::cout << std::endl;
      std::cout << "---" << std::endl;
      return false;
    }

    ia += num_incr_a;
    ib += num_incr_b;
  }

  if (ia == num_a && ib == num_b) {
    return true;
  }

  if (a.is_outputs()) {
    bool res = ib == num_b && ia == num_b;
    if (!res) {
      std::cout << "ERROR: Too many outputs requested." << std::endl;
    }
    return res;

  } else if (b.is_outputs()) {
    bool res = ia == num_a && ib == num_a;
    if (!res) {
      std::cout << "ERROR: Too many outputs requested." << std::endl;
    }
    return res;

  } else {
    std::cout << "Mismatching num: " << std::endl;
    type_printer().show2(a, b);
    std::cout << std::endl;
    assert(false);
    return false;
  }
}

bool Unifier::simplify_match_list(const TypeHandle& a,
                                  const std::vector<TypeHandle>& b,
                                  int64_t ib, int64_t* num_incr) {
  assert(ib < b.size());

  int64_t ia = 0;
  const int64_t num_a = store.at(a).list.size();
  const int64_t num_b = b.size();
  const int64_t b0 = ib;

  while (ia < num_a && ib < num_b) {
    assert(ib < b.size());

    const auto& pat_a = store.at(a).list.pattern[ia];
    const auto& mem_b = b[ib];

    if (simplify(pat_a, mem_b)) {
      ia = (ia + 1) % num_a;
      ib++;

    } else {
      break;
    }
  }

  //  True if all pattern members of `a` were visited.
  const bool match = num_a == 0 || (ia == 0 && ib > b0);

  if (match) {
    *num_incr = (ib - b0);
  }

  return match;
}

bool Unifier::simplify_match_destructured_tuple(const mt::types::DestructuredTuple& a,
                                                const std::vector<TypeHandle>& b,
                                                int64_t ib, int64_t* num_incr) {
  assert(ib < b.size());

  int64_t ia = 0;
  const int64_t num_a = a.members.size();
  const int64_t num_b = b.size();
  const int64_t b0 = ib;

  while (ia < num_a && ib < num_b) {
    const auto& mem_a = a.members[ia];
    const auto& mem_b = b[ib];

    if (simplify(mem_a, mem_b)) {
      ia++;
      ib++;
    } else {
      break;
    }
  }

  //  True if all pattern members of `a` were visited.
  const bool match = ia == num_a;

  if (match) {
    *num_incr = (ib - b0);
  }

  return match;
}

bool Unifier::simplify_match_num_rhs(const types::DestructuredTuple& t0, const types::DestructuredTuple& t1) {
  assert(t1.usage != types::DestructuredTuple::Usage::definition_inputs);

  const int64_t num_def = t0.members.size();
  const int64_t num_match = t1.members.size();

  if (num_def < num_match) {
    std::cout << "ERROR: More outputs requested than can be provided by definition: " << std::endl;
    type_printer().show2(t0, t1);
    std::cout << std::endl;
    assert(false);
    return false;

  } else {
    push_type_equations(t0.members, t1.members, num_match);
    return true;
  }
}

bool Unifier::simplify(const types::DestructuredTuple& t0, const types::DestructuredTuple& t1) {
  if (t0.usage == t1.usage) {
    return simplify_match_arguments(t0, t1);
  } else {
    return simplify_different_usage(t0, t1);
  }
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

  } else if (is_structured_tuple_type(sub.principal_argument)) {
    bool res = maybe_unify_subscript_tuple_principal_argument(source, sub);
    assert(res && "Failed to unify tuple primary subscript.");

  } else if (type_of(sub.principal_argument) != Type::Tag::variable) {
    std::cout << "Principal arg: ";
    type_printer().show(sub.principal_argument);
    std::cout << std::endl;
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
//    std::cout << "Non concrete: ";
//    type_printer().show(source);
//    std::cout << std::endl;
    return true;
  }

  std::vector<TypeHandle> into;
  auto args = sub0.arguments;
  args.insert(args.begin(), sub.principal_argument);

  DestructuredTuple tup(DestructuredTuple::Usage::rvalue, std::move(args));
//  flatten_destructured_tuple(tup, into);
//  tup.members = std::move(into);

  auto tup_type = store.make_type();
  store.assign(tup_type, Type(std::move(tup)));
  Abstraction abstraction(sub0.method, tup_type, TypeHandle());

  auto func_it = function_types.find(abstraction);
  if (func_it != function_types.end()) {
    const auto& func = store.at(func_it->second);
    assert(func.tag == Type::Tag::abstraction);

    const auto& func_outputs = func.abstraction.outputs;
    push_type_equation(TypeEquation(sub.outputs, func_outputs));

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
    std::cout << "ERROR: no such subscript signature: ";
    type_printer().show(tup_type);
    std::cout << std::endl;

    registered_funcs[source] = true;

    return false;
  }
}

bool Unifier::maybe_unify_subscript_tuple_principal_argument(const TypeHandle& source, types::Subscript& sub) {
  assert(sub.subscripts.size() == 1 && "Expected only 1 subscript.");
  const auto& sub0 = sub.subscripts[0];
  assert(sub0.method == SubscriptMethod::brace && "Only brace subscripts handled.");

  if (are_concrete_arguments(sub0.arguments)) {
    std::cout << "Concrete" << std::endl;
  }

  return true;
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
}

void Unifier::make_free_functions() {
  make_min();
  make_fileparts();
  make_list_outputs_type();
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

  store.assign(list_type,
    Type(List(List::Usage::definition, std::move(list_pattern))));
  store.assign(args_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_inputs, double_handle)));
  store.assign(result_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_outputs, std::move(outputs))));
  store.assign(func_type, Type(std::move(func)));

  function_types[func_copy] = func_type;
}

void Unifier::make_subscript_references() {
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
    store.assign(list_type, Type(List(List::Usage::definition, std::move(list_types))));
    store.assign(args_type,
      Type(DestructuredTuple(DestructuredTuple::Usage::definition_inputs, referent_type_handle, list_type)));
    store.assign(result_type,
      Type(DestructuredTuple(DestructuredTuple::Usage::definition_outputs, referent_type_handle)));

    store.assign(func_type, Type(std::move(ref)));

    function_types[ref_copy] = func_type;
    types_with_known_subscripts.insert(referent_type_handle);
  }
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
