#include "debug.hpp"
#include "typing.hpp"

namespace mt {

namespace {

const char* tuple_usage_shorthand(types::DestructuredTuple::Usage usage) {
  switch (usage) {
    case types::DestructuredTuple::Usage::definition_inputs:
      return "in";
    case types::DestructuredTuple::Usage::definition_outputs:
      return "out";
    case types::DestructuredTuple::Usage::rvalue:
      return "r";
    case types::DestructuredTuple::Usage::lvalue:
      return "l";
    default:
      assert(false && "Unhandled.");
  }
}

}

void DebugTypePrinter::show(const Type& t) const {
  using Tag = Type::Tag;

  switch (t.tag) {
    case Tag::scalar:
      show(t.scalar);
      break;
    case Tag::variable:
      show(t.variable);
      break;
    case Tag::abstraction:
      show(t.abstraction);
      break;
    case Tag::destructured_tuple:
      show(t.destructured_tuple);
      break;
    case Tag::tuple:
      show(t.tuple);
      break;
    case Tag::list:
      show(t.list);
      break;
    case Tag::subscript:
      show(t.subscript);
      break;
    default:
      assert(false && "Unhandled.");
  }
}

void DebugTypePrinter::show(const TypeHandle& handle) const {
  show(store.at(handle));
}

void DebugTypePrinter::show(const types::Scalar& scl) const {
  std::cout << "scl(" << scl.identifier.name << ")";
}

void DebugTypePrinter::show(const types::Variable& var) const {
  std::cout << "T" << var.identifier.name;
}

void DebugTypePrinter::show(const types::Abstraction& abstr) const {
  std::cout << "[";
  show(abstr.outputs);
  std::cout << "] = ";

  if (abstr.is_function()) {
    std::cout << string_registry.at(abstr.name.full_name());

  } else if (abstr.is_binary_operator()) {
    std::cout << to_string(abstr.binary_operator);

  } else if (abstr.type == types::Abstraction::Type::subscript_reference) {
    std::cout << ".";
  }

  std::cout << "(";
  show(abstr.inputs);
  std::cout << ")";
}

void DebugTypePrinter::show(const types::DestructuredTuple& tup) const {
  std::cout << "tp<" << tuple_usage_shorthand(tup.usage) << ">[";
  show(tup.members, ", ");
  std::cout << "]";
}

void DebugTypePrinter::show(const types::Tuple& tup) const {
  std::cout << "{";
  show(tup.members, ", ");
  std::cout << "}";
}

void DebugTypePrinter::show(const std::vector<TypeHandle>& handles, const char* delim) const {
  for (int64_t i = 0; i < handles.size(); i++) {
    show(handles[i]);
    if (i < handles.size()-1) {
      std::cout << ", ";
    }
  }
}

void DebugTypePrinter::show(const types::List& list) const {
  std::cout << "list[";
  show(list.pattern, ", ");
  std::cout << "]";
}

void DebugTypePrinter::show(const types::Subscript& subscript) const {
  std::cout << "()(";
  show(subscript.principal_argument);
  std::cout << "(";
  for (const auto& sub : subscript.subscripts) {
    show(sub.arguments, ", ");
  }
  std::cout << ")) -> ";
  show(subscript.outputs);
}

}
