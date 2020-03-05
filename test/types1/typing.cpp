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

void show_function_type(const types::Abstraction& func) {
  show_type_handles(func.outputs, "[", "]");
  std::cout << " = ";
  show_type_handles(func.inputs, "(", ")");
}

void show_type_eqs(const TypeVisitor& visitor, const std::vector<TypeEquation>& eqs) {
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

}

void Substitution::check_push_plus_func(const TypeHandle& source, const types::Abstraction& func) {
  if (func.inputs.size() == 2 && func.outputs.size() == 1 && registered_funcs.count(source) == 0) {
    push_type_equation(TypeEquation(source, require_plus_type_handle()));
    registered_funcs[source] = true;
  }
}

void Substitution::check_push_func(const mt::TypeHandle& source, const mt::types::Abstraction& func) {
  for (const auto& arg : func.inputs) {
    if (arg.get_index() != 0) {
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

TypeHandle Substitution::apply_to(const TypeHandle& source, types::Abstraction& func) {
  bool all_number = true;

  for (auto& arg : func.inputs) {
    arg = apply_to(arg);
    if (arg.get_index() != 0) {
      all_number = false;
    }
  }

  for (auto& out : func.outputs) {
    out = apply_to(out);
  }

  if (all_number) {
    check_push_plus_func(source, func);
  }

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
    default:
      assert(false);
  }
}

TypeHandle Substitution::substitute_one(types::Abstraction& func, const TypeHandle& source, const TypeHandle& lhs, const TypeHandle& rhs) {
  bool all_scalar = true;

  for (auto& arg : func.inputs) {
    arg = substitute_one(arg, lhs, rhs);
    if (type_of(arg) != DebugType::Tag::scalar) {
      all_scalar = false;
    }
  }

  for (auto& out: func.outputs) {
    out = substitute_one(out, lhs, rhs);
  }

  if (all_scalar) {
    check_push_func(source, func);
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
   *  func1(func2());
   *    r-value destructured tuple (1:1, func2) = r-value structured tuple (func1)
   *
   *  [a, b] = func1(func2());
   *    l-value destructured tuple t0 (1:2) = r-value destructured tuple t1 (1:2)
   *
   *  func1(a{:})
   *    r-value structured tuple t1 = r-value destructured tuple a
   *
   *  [a{:}] = func1()
   *    list
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
    default:
      assert(false);
  }
}

bool Substitution::simplify(const types::Scalar& t0, const types::Scalar& t1) {
  return t0.identifier.name == t1.identifier.name;
}

bool Substitution::simplify(const types::Abstraction& t0, const types::Abstraction& t1) {
  if (t0.inputs.size() != t1.inputs.size()) {
    return false;
  }
  if (t0.outputs.size() != t1.outputs.size()) {
    return false;
  }

  const int64_t num_inputs = t0.inputs.size();
  const int64_t num_outputs = t0.outputs.size();

  for (int64_t i = 0; i < num_inputs; i++) {
    push_type_equation(TypeEquation(t0.inputs[i], t1.inputs[i]));
  }
  for (int64_t i = 0; i < num_outputs; i++) {
    push_type_equation(TypeEquation(t0.outputs[i], t1.outputs[i]));
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
  types::Abstraction type(BinaryOperator::plus, arg, arg, arg);
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