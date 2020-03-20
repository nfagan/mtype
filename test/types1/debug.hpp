#pragma once

#include "type.hpp"
#include "type_store.hpp"

namespace mt {

class TypeVisitor;

class DebugTypePrinter {
public:
  explicit DebugTypePrinter(const TypeStore& store) : DebugTypePrinter(store, nullptr) {
    //
  }

  DebugTypePrinter(const TypeStore& store, const StringRegistry* string_registry) :
  store(store), string_registry(string_registry), colorize(true) {
    //
  }

  void show(const TypeHandle& handle) const;
  void show(const Type& type) const;
  void show(const types::Scalar& scl) const;
  void show(const types::Tuple& tup) const;
  void show(const types::Variable& var) const;
  void show(const types::Abstraction& abstr) const;
  void show(const types::DestructuredTuple& tup) const;
  void show(const types::List& list) const;
  void show(const types::Subscript& subscript) const;
  void show(const types::Scheme& scheme) const;
  void show(const types::Assignment& assignment) const;
  void show(const std::vector<TypeHandle>& handles, const char* delim) const;

  template <typename T, typename U>
  void show2(const T& a, const U& b) const {
    std::cout << "A: ";
    show(a);
    std::cout << std::endl << "B: ";
    show(b);
    std::cout << std::endl;
  }

private:
  const char* color(const char* color_code) const;
  std::string color(const std::string& color_code) const;
  const char* dflt_color() const;
  std::string list_color() const;

private:
  const TypeStore& store;
  const StringRegistry* string_registry;

public:
  bool colorize;
};

}