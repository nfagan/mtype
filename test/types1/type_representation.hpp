#pragma once

#include "type.hpp"
#include <sstream>

namespace mt {

class TypeStore;
class Library;

class TypeToString {
public:
  using DT = types::DestructuredTuple;

  explicit TypeToString(const TypeStore& store, const Library& library) : TypeToString(store, library, nullptr) {
    //
  }

  TypeToString(const TypeStore& store, const Library& library, const StringRegistry* string_registry) :
    store(store), library(library), string_registry(string_registry),
    rich_text(true), explicit_destructured_tuples(true), arrow_function_notation(false), max_num_type_variables(-1) {
    //
  }

  MT_NODISCARD std::string apply(const TypeHandle& handle) const;
  void apply(const TypeHandle& handle, std::stringstream& into) const;
  void apply(const Type& type, std::stringstream& into) const;
  void apply(const types::Scalar& scl, std::stringstream& into) const;
  void apply(const types::Tuple& tup, std::stringstream& into) const;
  void apply(const types::Variable& var, std::stringstream& into) const;
  void apply(const types::Abstraction& abstr, std::stringstream& into) const;
  void apply(const types::DestructuredTuple& tup, std::stringstream& into) const;
  void apply(const types::List& list, std::stringstream& into) const;
  void apply(const types::Subscript& subscript, std::stringstream& into) const;
  void apply(const types::Scheme& scheme, std::stringstream& into) const;
  void apply(const types::Assignment& assignment, std::stringstream& into) const;
  void apply(const std::vector<TypeHandle>& handles, std::stringstream& into, const char* delim = ", ") const;

private:
  void apply_implicit(const DT& tup, const Optional<DT::Usage>& parent_usage, std::stringstream& into) const;
  void apply_name(const types::Abstraction& abstr, std::stringstream& into) const;

  const char* color(const char* color_code) const;
  std::string color(const std::string& color_code) const;
  const char* dflt_color() const;
  std::string list_color() const;

private:
  const TypeStore& store;
  const Library& library;
  const StringRegistry* string_registry;

public:
  bool rich_text;
  bool explicit_destructured_tuples;
  bool arrow_function_notation;
  int max_num_type_variables;
};

}