#include "type_representation.hpp"
#include "type_store.hpp"
#include "library.hpp"
#include "mt/display.hpp"
#include <cassert>

namespace mt {

namespace {
  
const char* tuple_usage_shorthand(types::DestructuredTuple::Usage usage) {
  switch (usage) {
    case types::DestructuredTuple::Usage::definition_inputs:
      return "i";
    case types::DestructuredTuple::Usage::definition_outputs:
      return "o";
    case types::DestructuredTuple::Usage::rvalue:
      return "r";
    case types::DestructuredTuple::Usage::lvalue:
      return "l";
    default:
      assert(false && "Unhandled.");
      return "";
  }
}

}

void TypeToString::apply(const Type& t, std::stringstream& into) const {
  using Tag = Type::Tag;

  switch (t.tag) {
    case Tag::scalar:
      apply(t.scalar, into);
      break;
    case Tag::variable:
      apply(t.variable, into);
      break;
    case Tag::abstraction:
      apply(t.abstraction, into);
      break;
    case Tag::destructured_tuple:
      apply(t.destructured_tuple, into);
      break;
    case Tag::tuple:
      apply(t.tuple, into);
      break;
    case Tag::list:
      apply(t.list, into);
      break;
    case Tag::subscript:
      apply(t.subscript, into);
      break;
    case Tag::scheme:
      apply(t.scheme, into);
      break;
    case Tag::assignment:
      apply(t.assignment, into);
      break;
    default:
      assert(false && "Unhandled.");
  }
}

std::string TypeToString::apply(const TypeHandle& handle) const {
  std::stringstream into;
  apply(handle, into);
  return into.str();
}

void TypeToString::apply(const TypeHandle& handle, std::stringstream& into) const {
  apply(store.at(handle), into);
}

void TypeToString::apply(const types::Scalar& scl, std::stringstream& stream) const {
  const auto maybe_type_name = library.type_name(scl);
  stream << color(style::green);

  if (maybe_type_name) {
    stream << maybe_type_name.value();
  } else {
    stream << "s" << scl.identifier.name;
  }

  stream << dflt_color();
}

void TypeToString::apply(const types::Variable& var, std::stringstream& stream) const {
  stream << color(style::red) << "T" << var.identifier.name << dflt_color();
}

void TypeToString::apply(const types::Abstraction& abstr, std::stringstream& stream) const {
  if (arrow_function_notation) {
    apply_name(abstr, stream);

    stream << "(";
    apply(abstr.inputs, stream);
    stream << ")";

    if (rich_text) {
      stream << " → ";
    } else {
      stream << " -> ";
    }

    stream << "[";
    apply(abstr.outputs, stream);
    stream << "]";

  } else {
    stream << "[";
    apply(abstr.outputs, stream);
    stream << "] = ";

    apply_name(abstr, stream);

    stream << "(";
    apply(abstr.inputs, stream);
    stream << ")";
  }
}

void TypeToString::apply_name(const types::Abstraction& abstr, std::stringstream& stream) const {
  stream << color(style::yellow);

  if (abstr.is_function()) {
    if (string_registry) {
      stream << string_registry->at(abstr.name.full_name());
    } else {
      stream << "f-" << abstr.name.full_name();
    }

  } else if (abstr.is_binary_operator()) {
    stream << to_string(abstr.binary_operator);

  } else if (abstr.type == types::Abstraction::Type::subscript_reference) {
    stream << to_symbol(abstr.subscript_method);

  } else if (abstr.type == types::Abstraction::Type::concatenation) {
    stream << to_symbol(abstr.concatenation_direction);

  } else if (abstr.type == types::Abstraction::Type::anonymous_function) {
    stream << "@";
  }

  stream << dflt_color();
}

void TypeToString::apply(const types::DestructuredTuple& tup, std::stringstream& stream) const {
  if (!explicit_destructured_tuples) {
    apply_implicit(tup, NullOpt{}, stream);
  } else {
    stream << color(style::color_code(40)) << "dt-" << tuple_usage_shorthand(tup.usage);
    stream << dflt_color() << "[";
    apply(tup.members, stream, ", ");
    stream << "]";
  }
}

void TypeToString::apply(const types::Tuple& tup, std::stringstream& stream) const {
  stream << "{";
  apply(tup.members, stream, ", ");
  stream << "}";
}

void TypeToString::apply(const std::vector<TypeHandle>& handles, std::stringstream& stream, const char* delim) const {
  for (int64_t i = 0; i < handles.size(); i++) {
    apply(handles[i], stream);
    if (i < handles.size()-1) {
      stream << delim;
    }
  }
}

void TypeToString::apply(const types::List& list, std::stringstream& stream) const {
  stream << list_color() << "list" << dflt_color() << "[";
  apply(list.pattern, stream, ", ");
  stream << "]";
}

void TypeToString::apply(const types::Subscript& subscript, std::stringstream& stream) const {
  stream << "()(";
  apply(subscript.principal_argument, stream);
  stream << "(";
  for (const auto& sub : subscript.subscripts) {
    apply(sub.arguments, stream, ", ");
  }
  stream << ")) -> ";
  apply(subscript.outputs, stream);
}

void TypeToString::apply(const types::Scheme& scheme, std::stringstream& stream) const {
  stream << color(style::yellow) << "given" << dflt_color() << " <";
  if (max_num_type_variables < 0 || scheme.parameters.size() <= max_num_type_variables) {
    apply(scheme.parameters, stream, ", ");
  } else {
    stream << "...";
  }
  stream << "> ";
  apply(scheme.type, stream);
}

void TypeToString::apply(const types::Assignment& assignment, std::stringstream& stream) const {
  apply(assignment.lhs, stream);
  stream << " = ";
  apply(assignment.rhs, stream);
}

void TypeToString::apply_implicit(const DT& tup, const Optional<DT::Usage>& parent_usage, std::stringstream& stream) const {
  int64_t num_use = tup.members.size();

  if (num_use > 0 && tup.is_outputs() && parent_usage && DT::is_value_usage(parent_usage.value())) {
    num_use = 1;
  }

  for (int64_t i = 0; i < num_use; i++) {
    const auto& handle = tup.members[i];

    if (store.type_of(handle) == Type::Tag::destructured_tuple) {
      apply_implicit(store.at(handle).destructured_tuple, Optional<DT::Usage>(tup.usage), stream);
    } else {
      apply(handle, stream);
    }

    if (i < num_use-1) {
      stream << ", ";
    }
  }
}

const char* TypeToString::color(const char* color_code) const {
  return rich_text ? color_code : "";
}

std::string TypeToString::color(const std::string& color_code) const {
  return rich_text ? color_code : "";
}

const char* TypeToString::dflt_color() const {
  return color(style::dflt);
}

std::string TypeToString::list_color() const {
  return color(style::color_code(41));
}

}
