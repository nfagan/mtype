#pragma once

#include "types.hpp"
#include "../Optional.hpp"
#include <sstream>

namespace mt {

class TypeStore;
class Library;
class StringRegistry;

class TypeToString {
public:
  using DT = types::DestructuredTuple;

  explicit TypeToString(const Library* library) : TypeToString(library, nullptr) {
    //
  }

  TypeToString(const Library* library, const StringRegistry* string_registry);

  MT_NODISCARD std::string apply(const Type* t) const;

  void apply(const Type* handle, std::stringstream& into) const;
  void apply(const types::Scalar& scl, std::stringstream& into) const;
  void apply(const types::Tuple& tup, std::stringstream& into) const;
  void apply(const types::Variable& var, std::stringstream& into) const;
  void apply(const types::Abstraction& abstr, std::stringstream& into) const;
  void apply(const types::Application& app, std::stringstream& into) const;
  void apply(const types::DestructuredTuple& tup, std::stringstream& into) const;
  void apply(const types::List& list, std::stringstream& into) const;
  void apply(const types::Subscript& subscript, std::stringstream& into) const;
  void apply(const types::Scheme& scheme, std::stringstream& into) const;
  void apply(const types::Assignment& assignment, std::stringstream& into) const;
  void apply(const types::Parameters& params, std::stringstream& into) const;
  void apply(const types::Union& union_type, std::stringstream& into) const;
  void apply(const types::ConstantValue& val, std::stringstream& into) const;
  void apply(const types::Class& cls, std::stringstream& into) const;
  void apply(const types::Record& record, std::stringstream& into) const;
  void apply(const types::Alias& alias, std::stringstream& into) const;
  void apply(const TypePtrs& handles, std::stringstream& into, const char* delim = ", ") const;

  const char* color(const char* color_code) const;
  std::string color(const std::string& color_code) const;
  const char* dflt_color() const;
  std::string list_color() const;
  std::string class_color() const;
  std::string constant_color() const;
  const char* right_arrow() const;

private:
  void apply_implicit(const DT& tup, const Optional<DT::Usage>& parent_usage, std::stringstream& into) const;
  void apply_name(const types::Abstraction& abstr, std::stringstream& into) const;

private:
  const Library* library;
  const StringRegistry* string_registry;

public:
  bool rich_text;
  bool explicit_destructured_tuples;
  bool explicit_aliases;
  bool arrow_function_notation;
  bool show_class_source_type;
  int max_num_type_variables;
};

}