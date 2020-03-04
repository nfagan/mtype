#include "typing.hpp"

namespace mt {

namespace {

template <typename T>
void show_type_handle(const T& handle) {
  std::cout << "T" << handle.get_index();
}

void show_function_type(const types::Function& func) {
  std::cout << "(";
  for (const auto& arg : func.inputs) {
    show_type_handle(arg);
    std::cout << ",";
  }
  assert(!func.outputs.empty());
  std::cout << ") -> ";
  show_type_handle(func.outputs[0]);
}

void show_type_eqs(const TypeVisitor& visitor, const std::vector<TypeEquation>& eqs) {
  std::cout << "====" << std::endl;
  for (const auto& eq : eqs) {
    const auto& left_type = visitor.at(eq.lhs);
    const auto& right_type = visitor.at(eq.rhs);

    show_type_handle(eq.lhs);
    std::cout << " = ";
    show_type_handle(eq.rhs);

    if (right_type.tag == DebugType::Tag::function) {
      std::cout << " ";
      show_function_type(right_type.function);
    } else if (right_type.tag == DebugType::Tag::scalar) {
      std::cout << " (sclr)";
    } else if (right_type.tag == DebugType::Tag::variable) {
      std::cout << " (var)";
    }

    std::cout << std::endl;
  }

  std::cout << "====" << std::endl;
}

void show_app_type(const texpr::Application& app) {
  std::cout << "(";
  for (const auto& arg : app.arguments) {
    show_type_handle(arg);
    std::cout << ",";
  }
  assert(!app.outputs.empty());
  std::cout << ") -> ";
  show_type_handle(app.outputs[0]);
  std::cout << std::endl;
}

}

void Substitution::unify() {
  while (!type_equations.empty()) {
    auto eq = type_equations.front();
    type_equations.erase(type_equations.begin());
    unify_one(eq);
  }

  show();
}

void Substitution::substitute_variable(TypeHandle& var_handle, mt::types::Variable& var) {
  if (bound_variables.count(var_handle) > 0) {
    var_handle = bound_variables.at(var_handle);
  }
}

void Substitution::substitute_function(types::Function& func) {
  bool all_number = true;

  for (auto& arg : func.inputs) {
    substitute_dispatch(arg);
    if (arg.get_index() != 0) {
      all_number = false;
    }
  }

  for (auto& out : func.outputs) {
    substitute_dispatch(out);
  }

  if (all_number && !func.outputs.empty() && !func.inputs.empty()) {
    if (func.outputs[0].get_index() != 0) {
      type_equations.emplace_back(TypeEquation(func.outputs[0], func.inputs[0]));
    }
  }
}

void Substitution::substitute_dispatch(mt::TypeHandle& handle) {
  auto& type = visitor.at(handle);

  switch (type.tag) {
    case DebugType::Tag::variable:
      substitute_variable(handle, type.variable);
      break;
    case DebugType::Tag::scalar:
      break;
    case DebugType::Tag::function:
      substitute_function(type.function);
      break;
    default:
      assert(false);
  }
}

void Substitution::unify_one(mt::TypeEquation eq) {
  using Tag = DebugType::Tag;

  auto lhs_type = type_of(eq.lhs);
  auto rhs_type = type_of(eq.rhs);

  if (rhs_type == Tag::variable) {
    const bool has_rhs_var = visitor.variables.count(eq.rhs) > 0;
    if (lhs_type != Tag::variable || !has_rhs_var) {
      std::swap(eq.lhs, eq.rhs);
      rhs_type = visitor.at(eq.rhs).tag;
    }
  }

  substitute_dispatch(eq.lhs);
  substitute_dispatch(eq.rhs);

  if (eq.lhs == eq.rhs) {
    return;
  }

  lhs_type = type_of(eq.lhs);
  rhs_type = type_of(eq.rhs);

  if (lhs_type != Tag::variable && rhs_type != Tag::variable) {
    if (eq.lhs != eq.rhs) {
      std::cout << "Error unlike types" << std::endl;
    }
    return;
//    assert(false && "Unhandled.");
  } else if (lhs_type != Tag::variable) {
    assert(false && "Unhandled");
  }

  assert(bound_variables.count(eq.lhs) == 0);
  bound_variables[eq.lhs] = eq.rhs;

  for (auto& subst_eq : substitution) {
    substitute_dispatch(subst_eq.rhs);
    assert(type_of(subst_eq.lhs) == DebugType::Tag::variable);
  }

  substitution.push_back(eq);
}


DebugType::Tag Substitution::type_of(const mt::TypeHandle& handle) const {
  return visitor.at(handle).tag;
}

void Substitution::show() {
  show_type_eqs(visitor, type_equations);
  show_type_eqs(visitor, substitution);

  std::cout << "----" << std::endl;
  for (const auto& t : visitor.types) {
    if (t.tag == DebugType::Tag::function) {
      show_function_type(t.function);
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


//    auto& subst_rhs = visitor.at(subst_eq.rhs);

//    switch (subst_rhs.tag) {
//      case Tag::variable: {
//        if (subst_eq.rhs == eq.lhs) {
//          std::cout << "Simplify" << std::endl;
////          subst_eq.rhs = eq.rhs;
//        }
//        break;
//      }
//      case Tag::scalar:
////        bound_variables[subst_eq.lhs] = subst_eq.rhs;
//        break;
//      case Tag::function: {
//        substitute_function(subst_rhs.function, bound_variables);
//        break;
//      }
//      default:
//        assert(false);
//    }