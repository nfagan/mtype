#include "typing.hpp"

namespace mt {

namespace {
void show_type_eqs(const TypeVisitor& visitor, const std::vector<TypeEquation>& eqs) {
  std::cout << "====" << std::endl;
  for (const auto& eq : eqs) {
    const auto& left_type = visitor.at(eq.lhs);
    const auto& right_type = visitor.at(eq.rhs);

    std::cout << "T" << eq.lhs.get_index() << " = " << "T" << eq.rhs.get_index();

    if (right_type.tag == DebugType::Tag::function) {
      assert(!right_type.function.outputs.empty());
      std::cout << "; Result: T" << right_type.function.outputs[0].get_index();
    } else if (right_type.tag == DebugType::Tag::scalar) {
      std::cout << " (sclr)";
    }

    std::cout << std::endl;
  }

  std::cout << "====" << std::endl;
}

void show_app_type(const texpr::Application& app) {
  std::cout << "(";
  for (const auto& arg : app.arguments) {
    std::cout << "T" << arg.get_index() << ",";
  }
  std::cout << ") -> T" << app.outputs[0].get_index() << std::endl;
  std::cout << std::endl;
}

}

void Substitution::unify() {
  unify_new();

  show_type_eqs(visitor, type_equations);
  show_type_eqs(visitor, substitution);

  for (const auto& app_it : visitor.applications) {
    texpr::Application app_copy(app_it.first);

    for (auto& arg : app_copy.arguments) {
      if (bound_variables.count(arg) > 0) {
        arg = bound_variables.at(arg);
      }
    }
    for (auto& out : app_copy.outputs) {
      if (bound_variables.count(out) > 0) {
        out = bound_variables.at(out);
      }
    }

    show_app_type(app_copy);
  }
}

void Substitution::unify_one(mt::TypeEquation eq) {
  auto rhs_type = visitor.at(eq.rhs).tag;

  if (rhs_type == DebugType::Tag::variable) {
    std::swap(eq.lhs, eq.rhs);
    rhs_type = visitor.at(eq.rhs).tag;
  }

  switch (rhs_type) {
    case DebugType::Tag::variable:
      if (bound_variables.count(eq.rhs) > 0) {
        eq.rhs = bound_variables.at(eq.rhs);
      }
      break;
    case DebugType::Tag::scalar:
      break;
    default:
      assert(false);
  }

  bound_variables[eq.lhs] = eq.rhs;

  for (auto& subst_eq : substitution) {
    const auto& subst_rhs = visitor.at(subst_eq.rhs);

    switch (subst_rhs.tag) {
      case DebugType::Tag::variable:
        if (subst_eq.rhs == eq.lhs) {
          subst_eq.rhs = eq.rhs;
        }
        break;
      case DebugType::Tag::scalar:
        bound_variables[subst_eq.lhs] = subst_eq.rhs;
        break;
      default:
        assert(false);
    }
  }

  substitution.push_back(eq);
}

void Substitution::unify_new() {
  for (const auto& eq : type_equations) {
    unify_one(eq);
  }
}

}
