#pragma once

#include "typing.hpp"

namespace mt {

class TypeExprStringVisitor : public TypeExprVisitor {
public:
  TypeExprStringVisitor(const TypeVisitor& type_visitor) :
  type_visitor(type_visitor), substitution(type_visitor.substitution) {
    //
  }

  void application(const texpr::Application& app) override {
    assert(app.type == texpr::Application::Type::binary_operator);
    std::string result("(");
    if (app.type == texpr::Application::Type::binary_operator) {
      result += "<op>";
    }
    for (const auto& arg : app.arguments) {
      result += type_handle_str(arg);
      result += ",";
    }
    result += ")";
    append(result);
  }

  void show() {
//    for (const auto& eq : substitution.type_equations) {
//      auto eq_str = type_handle_str(eq.lhs);
//      eq_str += " = ";
//      eq_str += type_handle_str(eq.rhs);
//      eq_str += " = ";
//      const auto& rhs = type_visitor.at(eq.rhs);
//      std::cout << "Is variable? " << rhs.is_variable() << std::endl;
//      eq_str += str;
//      str.clear();
//      std::cout << eq_str << std::endl;
//    }
  }

private:
  static std::string type_expr_handle_str(const TypeExprHandle& handle) {
    return "T" + std::to_string(handle.get_index());
  }
  static std::string type_handle_str(const TypeHandle& handle) {
    return "T" + std::to_string(handle.get_index());
  }

  void append(const std::string& s) {
    str += s;
  }

private:
  const TypeVisitor& type_visitor;
  const Substitution& substitution;

  std::string str;
};

}