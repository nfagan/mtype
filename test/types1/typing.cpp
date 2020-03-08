#include "typing.hpp"

namespace mt {

namespace {

template <typename T>
void show_type_handle(const T& handle) {
  std::cout << "T" << handle.get_index();
}

template <typename T>
void show_type_handles(const std::vector<T>& handles, const char* open, const char* close) {
  std::cout << open;
  for (int64_t i = 0; i < handles.size(); i++) {
    show_type_handle(handles[i]);
    if (i < handles.size()-1) {
      std::cout << ", ";
    }
  }
  std::cout << close;
}

}

void Substitution::show_function_type(const types::Abstraction& func) const {
  const auto& outputs = visitor.at(func.outputs);
  assert(outputs.tag == DebugType::Tag::tuple);
  const auto& inputs = visitor.at(func.inputs);
  assert(inputs.tag == DebugType::Tag::tuple);

  show_type_handles(outputs.tuple.members, "[", "]");
  std::cout << " = ";
  show_type_handles(inputs.tuple.members, "(", ")");
}

void Substitution::show_type_eqs(const std::vector<TypeEquation>& eqs) const {
  std::cout << "====" << std::endl;
  for (const auto& eq : eqs) {
    const auto& left_type = visitor.at(eq.lhs);
    const auto& right_type = visitor.at(eq.rhs);

    show_type_handle(eq.lhs);
    std::cout << " = ";
    show_type_handle(eq.rhs);

    if (right_type.tag == DebugType::Tag::abstraction) {
      std::cout << " ";
      show_function_type(right_type.abstraction);
    } else if (right_type.tag == DebugType::Tag::scalar) {
      std::cout << " (sclr)";
    } else if (right_type.tag == DebugType::Tag::variable) {
      std::cout << " (var)";
    }

    std::cout << std::endl;
  }

  std::cout << "====" << std::endl;
}

void Substitution::check_push_plus_func(const TypeHandle& source, const types::Abstraction& func) {
  const auto& inputs = visitor.at(func.inputs);
  const auto& outputs = visitor.at(func.outputs);
  assert(inputs.tag == DebugType::Tag::tuple && outputs.tag == DebugType::Tag::tuple);

  const auto& input_members = inputs.tuple.members;
  const auto& output_members = outputs.tuple.members;

  if (input_members.size() == 2 && output_members.size() == 1 && registered_funcs.count(source) == 0) {
    push_type_equation(TypeEquation(source, require_plus_type_handle()));
    registered_funcs[source] = true;
  }
}

void Substitution::check_push_func(const TypeHandle& source, const types::Abstraction& func) {
  const auto& inputs = visitor.at(func.inputs);
  assert(inputs.tag == DebugType::Tag::tuple);

  for (const auto& arg : inputs.tuple.members) {
    if (type_of(arg) != DebugType::Tag::scalar) {
      return;
    } else if (arg.get_index() != 0) {
      std::cout << "Err no such function" << std::endl;
      return;
    }
  }

  check_push_plus_func(source, func);
}

void Substitution::unify() {
  while (!type_equations.empty()) {
    auto eq = type_equations.back();
    type_equations.pop_back();
    unify_one(eq);
  }

  show();
}

TypeHandle Substitution::apply_to(const TypeHandle& source, mt::types::Variable& var) {
  if (bound_variables.count(source) > 0) {
    return bound_variables.at(source);
  } else {
    return source;
  }
}

TypeHandle Substitution::apply_to(const TypeHandle& source, types::Tuple& tup) {
  for (auto& member : tup.members) {
    member = apply_to(member);
  }
  return source;
}

TypeHandle Substitution::apply_to(const TypeHandle& source, types::Abstraction& func) {
  func.inputs = apply_to(func.inputs);
  func.outputs = apply_to(func.outputs);

  check_push_func(source, func);

  return source;
}

TypeHandle Substitution::apply_to(const TypeHandle& source) {
  auto& type = visitor.at(source);

  switch (type.tag) {
    case DebugType::Tag::variable:
      return apply_to(source, type.variable);
    case DebugType::Tag::scalar:
      return source;
    case DebugType::Tag::abstraction:
      return apply_to(source, type.abstraction);
    case DebugType::Tag::tuple:
      return apply_to(source, type.tuple);
    default:
      assert(false);
  }
}

TypeHandle Substitution::substitute_one(const TypeHandle& source, const TypeHandle& lhs, const TypeHandle& rhs) {
  auto& type = visitor.at(source);

  switch (type.tag) {
    case DebugType::Tag::variable:
      return substitute_one(type.variable, source, lhs, rhs);
    case DebugType::Tag::scalar:
      return source;
    case DebugType::Tag::abstraction:
      return substitute_one(type.abstraction, source, lhs, rhs);
    case DebugType::Tag::tuple:
      return substitute_one(type.tuple, source, lhs, rhs);
    default:
      assert(false);
  }
}

TypeHandle Substitution::substitute_one(types::Abstraction& func, const TypeHandle& source, const TypeHandle& lhs, const TypeHandle& rhs) {
  func.inputs = substitute_one(func.inputs, lhs, rhs);
  func.outputs = substitute_one(func.outputs, lhs, rhs);

  check_push_func(source, func);

  return source;
}

TypeHandle Substitution::substitute_one(types::Tuple& tup, const TypeHandle& source, const TypeHandle& lhs, const TypeHandle& rhs) {
  for (auto& member : tup.members) {
    member = substitute_one(member, lhs, rhs);
  }

  return source;
}

TypeHandle Substitution::substitute_one(types::Variable& var, const TypeHandle& source, const TypeHandle& lhs, const TypeHandle& rhs) {
  if (source == lhs) {
    return rhs;
  } else {
    return source;
  }
}

void Substitution::unify_one(TypeEquation eq) {
  using Tag = DebugType::Tag;

  eq.lhs = apply_to(eq.lhs);
  eq.rhs = apply_to(eq.rhs);

  if (eq.lhs == eq.rhs) {
    return;
  }

  const auto lhs_type = type_of(eq.lhs);
  const auto rhs_type = type_of(eq.rhs);

  if (lhs_type != Tag::variable && rhs_type != Tag::variable) {
    bool success = simplify(eq.lhs, eq.rhs);
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

bool Substitution::simplify(const TypeHandle& lhs, const TypeHandle& rhs) {
  /*
   *  For each element of a destructured tuple, consider the number of outputs that it produces.
   *  [a, b{:}]: num(a) == 1, num(b{:})
   *
   *  Number of outputs must be known at compile time: [a{:}, b{:}] = func1() is only valid
   *  if a and b are tuples, not lists.
   *
   *  Difference between assignment to list / tuple: [a{:}] = deal( 1 );
   *  and variable [a(1:10), b(1:10)] = 1;
   *
   *  function [a, b, c, d] = func1(e)
   *    a = b = c = d = e = 1;
   *  end
   *
   *  let A = {number, number, number}
   *  let B = {number}
   *
   *  a: A = {1, 2, 3}
   *  b: B = {1}
   *
   *  [a{:}, b{:}] = func1(); //  ok
   *
   *  func1(func2());
   *    r-value destructured tuple (1:1, func2) = r-value structured tuple (func1)
   *
   *  [a, b] = func1(func2());
   *    l-value destructured tuple t0 (1:2) = r-value destructured tuple t1 (1:2)
   *
   *  func1(a{:})
   *    r-value structured tuple t1 = r-value destructured tuple a
   *
   *  [a.b.c{1:2}] = func1()
   *    destructured list c{1:2} = structured tuple t1
   *  [a{1:2}, b] = func1()
   *
   *  func1(a{:})
   *  func1([a{:}, b{:}])
   */
  using Tag = DebugType::Tag;

  if (type_of(lhs) != type_of(rhs)) {
    return false;
  }

  const auto& t0 = visitor.at(lhs);
  const auto& t1 = visitor.at(rhs);

  switch (t0.tag) {
    case Tag::abstraction:
      return simplify(t0.abstraction, t1.abstraction);
    case Tag::scalar:
      return simplify(t0.scalar, t1.scalar);
    case Tag::tuple:
      return simplify(t0.tuple, t1.tuple);
    default:
      assert(false);
  }
}

bool Substitution::simplify(const types::Scalar& t0, const types::Scalar& t1) {
  return t0.identifier.name == t1.identifier.name;
}

bool Substitution::simplify(const types::Abstraction& t0, const types::Abstraction& t1) {
  if (!simplify(t0.inputs, t1.inputs)) {
    return false;
  }

  if (!simplify(t0.outputs, t1.outputs)) {
    return false;
  }

  return true;
}

bool Substitution::simplify(const types::Tuple& t0, const types::Tuple& t1) {
  if (t0.members.size() != t1.members.size()) {
    return false;
  }
  const int64_t num_members = t0.members.size();
  for (int64_t i = 0; i < num_members; i++) {
    push_type_equation(TypeEquation(t0.members[i], t1.members[i]));
  }
  return true;
}

DebugType::Tag Substitution::type_of(const mt::TypeHandle& handle) const {
  return visitor.at(handle).tag;
}

TypeHandle Substitution::require_plus_type_handle() {
  if (plus_type_handle.is_valid()) {
    return plus_type_handle;
  }

  const auto& arg = visitor.double_type_handle;
  plus_type_handle = visitor.make_type();

  const auto inputs_type_handle = visitor.make_type();
  const auto outputs_type_handle = visitor.make_type();
  visitor.assign(inputs_type_handle, DebugType(types::Tuple(types::Tuple::Structuring::structured, arg, arg)));
  visitor.assign(outputs_type_handle, DebugType(types::Tuple(types::Tuple::Structuring::destructured, arg)));

  types::Abstraction type(BinaryOperator::plus, inputs_type_handle, outputs_type_handle);
  visitor.assign(plus_type_handle, DebugType(std::move(type)));

  return plus_type_handle;
}

void Substitution::show() {
  std::cout << "----" << std::endl;
  for (const auto& t : visitor.types) {
    if (t.tag == DebugType::Tag::abstraction) {
      show_function_type(t.abstraction);
      std::cout << std::endl;
    }
  }
  std::cout << "----" << std::endl;
  for (const auto& it : visitor.variables) {
    int64_t var_name;
    {
      Store::ReadConst reader(visitor.store);
      var_name = reader.at(it.second).name.full_name();
    }

    show_type_handle(it.first);
    std::cout << " | " << visitor.string_registry.at(var_name) << std::endl;
  }
}

}