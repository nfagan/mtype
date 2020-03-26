#pragma once

#include "type.hpp"
#include "type_store.hpp"
#include "type_representation.hpp"
#include "library.hpp"

namespace mt {

class DebugTypePrinter {
public:
  explicit DebugTypePrinter(const TypeStore& store) : DebugTypePrinter(store, nullptr, nullptr) {
    //
  }

  DebugTypePrinter(const TypeStore& store, const Library* library, const StringRegistry* string_registry) :
  store(store), library(library), string_registry(string_registry), colorize(true) {
    //
  }

  template <typename T>
  void show(const T& a) const {
    std::stringstream str;
    to_string_impl().apply(a, str);
    std::cout << str.str();
  }

  template <typename T, typename U>
  void show2(const T& a, const U& b) const {
    std::cout << "A: ";
    show(a);
    std::cout << std::endl << "B: ";
    show(b);
    std::cout << std::endl;
  }

private:
  TypeToString to_string_impl() const {
    TypeToString impl(store, library, string_registry);
    impl.rich_text = colorize;
    return impl;
  }

private:
  const TypeStore& store;
  const Library* library;
  const StringRegistry* string_registry;

public:
  bool colorize;
};

}