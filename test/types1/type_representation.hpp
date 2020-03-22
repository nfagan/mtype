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
    store(store), library(library), string_registry(string_registry), colorize(true), explicit_destructured_tuples(true) {
    //
  }

  void clear();
  std::string str() const;

  void apply(const TypeHandle& handle);
  void apply(const Type& type);
  void apply(const types::Scalar& scl);
  void apply(const types::Tuple& tup);
  void apply(const types::Variable& var);
  void apply(const types::Abstraction& abstr);
  void apply(const types::DestructuredTuple& tup);
  void apply(const types::List& list);
  void apply(const types::Subscript& subscript);
  void apply(const types::Scheme& scheme);
  void apply(const types::Assignment& assignment);
  void apply(const std::vector<TypeHandle>& handles, const char* delim = ", ");

private:
  void apply_implicit(const DT& tup, const Optional<DT::Usage>& parent_usage);

  const char* color(const char* color_code) const;
  std::string color(const std::string& color_code) const;
  const char* dflt_color() const;
  std::string list_color() const;

private:
  std::stringstream stream;
  const TypeStore& store;
  const Library& library;
  const StringRegistry* string_registry;

public:
  bool colorize;
  bool explicit_destructured_tuples;
};

}