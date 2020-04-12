#include "type_representation.hpp"
#include "library.hpp"
#include "../display.hpp"
#include "../string.hpp"
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

void TypeToString::apply(const Type* t, std::stringstream& into) const {
  t->accept(*this, into);
}

std::string TypeToString::apply(const Type* t) const {
  std::stringstream into;
  apply(t, into);
  return into.str();
}

void TypeToString::apply(const types::Scalar& scl, std::stringstream& stream) const {
  Optional<std::string> maybe_type_name;

  if (library) {
    maybe_type_name = library->type_name(&scl);
  }

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

    stream << " " << right_arrow() << " ";

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
    stream << to_symbol(abstr.binary_operator);

  } else if (abstr.is_unary_operator()) {
    stream << to_symbol(abstr.unary_operator);

  } else if (abstr.kind == types::Abstraction::Kind::subscript_reference) {
    stream << to_symbol(abstr.subscript_method);

  } else if (abstr.kind == types::Abstraction::Kind::concatenation) {
    stream << to_symbol(abstr.concatenation_direction);

  } else if (abstr.kind == types::Abstraction::Kind::anonymous_function) {
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

void TypeToString::apply(const types::Union& union_type, std::stringstream& into) const {
  apply(union_type.members, into, " | ");
}

void TypeToString::apply(const types::ConstantValue& val, std::stringstream& into) const {
  into << constant_color();
  switch (val.kind) {
    case types::ConstantValue::Kind::int_value:
      into << val.int_value;
      break;
    case types::ConstantValue::Kind::double_value:
      into << val.double_value;
      break;
    case types::ConstantValue::Kind::char_value:
      into << "\"";
      if (string_registry) {
        into << string_registry->at(val.char_value.full_name());
      }
      into << "\"";
      break;
  }
  into << dflt_color();
}

void TypeToString::apply(const types::Class& cls, std::stringstream& into) const {
  std::string name;
  if (string_registry) {
    name = string_registry->at(cls.name.full_name());
  } else {
    name = "(class)";
  }

  into << class_color() << name << dflt_color();

  if (show_class_source_type) {
    apply(cls.source, into);
  }
}

void TypeToString::apply(const types::Record& record, std::stringstream& into) const {
  into << "{";
  int64_t i = 0;
  for (const auto& field : record.fields) {
    apply(field.name, into);
    into << ": ";
    apply(field.type, into);

    if (i++ < int64_t(record.fields.size()-1)) {
      into << ", ";
    }
  }
  into << "}";
}

void TypeToString::apply(const TypePtrs& handles, std::stringstream& stream, const char* delim) const {
  const int64_t sz = handles.size();
  for (int64_t i = 0; i < sz; i++) {
    apply(handles[i], stream);
    if (i < sz-1) {
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
  apply(subscript.principal_argument, stream);
  for (const auto& sub : subscript.subscripts) {
    stream << to_symbol(sub.method) << "(";
    apply(sub.arguments, stream, ", ");
    stream << ")";
  }

  stream << " " << right_arrow() << " ";
  apply(subscript.outputs, stream);
}

void TypeToString::apply(const types::Scheme& scheme, std::stringstream& stream) const {
  stream << color(style::yellow) << "given" << dflt_color() << " <";
  if (max_num_type_variables < 0 || int(scheme.parameters.size()) <= max_num_type_variables) {
    apply(scheme.parameters, stream, ", ");
  } else {
    stream << color(style::red) << scheme.parameters.size() << dflt_color();
  }
  stream << "> ";
  apply(scheme.type, stream);
}

void TypeToString::apply(const types::Assignment& assignment, std::stringstream& stream) const {
  apply(assignment.lhs, stream);
  stream << " = ";
  apply(assignment.rhs, stream);
}

void TypeToString::apply(const types::Parameters& params, std::stringstream& stream) const {
  stream << color(style::red) << "P" << params.identifier.name << dflt_color();
}

void TypeToString::apply_implicit(const DT& tup, const Optional<DT::Usage>& parent_usage, std::stringstream& stream) const {
  int64_t num_use = tup.members.size();

  if (num_use > 0 && tup.is_outputs() && parent_usage && DT::is_value_usage(parent_usage.value())) {
    num_use = 1;
  }

  for (int64_t i = 0; i < num_use; i++) {
    const auto& handle = tup.members[i];

    if (handle->is_destructured_tuple()) {
      apply_implicit(MT_DT_REF(*handle), Optional<DT::Usage>(tup.usage), stream);
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

std::string TypeToString::class_color() const {
  return color(style::color_code(43));
}

std::string TypeToString::constant_color() const {
  return "";
}

const char* TypeToString::right_arrow() const {
  return rich_text ? "→" : "->";
}

}
