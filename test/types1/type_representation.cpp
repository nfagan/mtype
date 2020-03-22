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

void TypeToString::clear() {
  stream.str(std::string());
}

std::string TypeToString::str() const {
  return stream.str();
}

void TypeToString::apply(const Type& t) {
  using Tag = Type::Tag;

  switch (t.tag) {
    case Tag::scalar:
      apply(t.scalar);
      break;
    case Tag::variable:
      apply(t.variable);
      break;
    case Tag::abstraction:
      apply(t.abstraction);
      break;
    case Tag::destructured_tuple:
      apply(t.destructured_tuple);
      break;
    case Tag::tuple:
      apply(t.tuple);
      break;
    case Tag::list:
      apply(t.list);
      break;
    case Tag::subscript:
      apply(t.subscript);
      break;
    case Tag::scheme:
      apply(t.scheme);
      break;
    case Tag::assignment:
      apply(t.assignment);
      break;
    default:
      assert(false && "Unhandled.");
  }
}

void TypeToString::apply(const TypeHandle& handle) {
  apply(store.at(handle));
}

void TypeToString::apply(const types::Scalar& scl) {
  const auto maybe_type_name = library.type_name(scl);
  stream << color(style::green);

  if (maybe_type_name) {
    stream << maybe_type_name.value();
  } else {
    stream << "s" << scl.identifier.name;
  }

  stream << dflt_color();
}

void TypeToString::apply(const types::Variable& var) {
  stream << color(style::red) << "T" << var.identifier.name << dflt_color();
}

void TypeToString::apply(const types::Abstraction& abstr) {
  stream << "[";
  apply(abstr.outputs);
  stream << "] = " << color(style::yellow);

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

  stream << dflt_color() << "(";
  apply(abstr.inputs);
  stream << ")";
}

void TypeToString::apply(const types::DestructuredTuple& tup) {
  if (!explicit_destructured_tuples) {
    apply_implicit(tup, NullOpt{});
  } else {
    stream << color(style::color_code(40)) << "dt-" << tuple_usage_shorthand(tup.usage);
    stream << dflt_color() << "[";
    apply(tup.members, ", ");
    stream << "]";
  }
}

void TypeToString::apply(const types::Tuple& tup) {
  stream << "{";
  apply(tup.members, ", ");
  stream << "}";
}

void TypeToString::apply(const std::vector<TypeHandle>& handles, const char* delim) {
  for (int64_t i = 0; i < handles.size(); i++) {
    apply(handles[i]);
    if (i < handles.size()-1) {
      stream << delim;
    }
  }
}

void TypeToString::apply(const types::List& list) {
  stream << list_color() << "list" << dflt_color() << "[";
  apply(list.pattern, ", ");
  stream << "]";
}

void TypeToString::apply(const types::Subscript& subscript) {
  stream << "()(";
  apply(subscript.principal_argument);
  stream << "(";
  for (const auto& sub : subscript.subscripts) {
    apply(sub.arguments, ", ");
  }
  stream << ")) -> ";
  apply(subscript.outputs);
}

void TypeToString::apply(const types::Scheme& scheme) {
  stream << "given<";
  apply(scheme.parameters, ", ");
  stream << ">";
  apply(scheme.type);
}

void TypeToString::apply(const types::Assignment& assignment) {
  apply(assignment.lhs);
  stream << " = ";
  apply(assignment.rhs);
}

void TypeToString::apply_implicit(const DT& tup, const Optional<DT::Usage>& parent_usage) {
  int64_t num_use = tup.members.size();

  if (num_use > 0 && tup.is_outputs() && parent_usage && DT::is_value_usage(parent_usage.value())) {
    num_use = 1;
  }

  for (int64_t i = 0; i < num_use; i++) {
    const auto& handle = tup.members[i];

    if (store.type_of(handle) == Type::Tag::destructured_tuple) {
      apply_implicit(store.at(handle).destructured_tuple, Optional<DT::Usage>(tup.usage));
    } else {
      apply(handle);
    }

    if (i < num_use-1) {
      stream << ", ";
    }
  }
}

const char* TypeToString::color(const char* color_code) const {
  return colorize ? color_code : "";
}

std::string TypeToString::color(const std::string& color_code) const {
  return colorize ? color_code : "";
}

const char* TypeToString::dflt_color() const {
  return color(style::dflt);
}

std::string TypeToString::list_color() const {
  return color(style::color_code(41));
}

}
