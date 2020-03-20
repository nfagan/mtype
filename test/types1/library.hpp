#pragma once

#include "type.hpp"
#include "type_equality.hpp"
#include <map>

namespace mt {

class TypeStore;

class Library {
  friend class Unifier;
public:
  explicit Library(TypeStore& store, const TypeEquality& type_eq, StringRegistry& string_registry) :
  store(store),
  string_registry(string_registry),
  arg_comparator(type_eq),
  type_equiv_comparator(type_eq),
  function_types(arg_comparator),
  types_with_known_subscripts(type_equiv_comparator) {
    //
  }

  void make_known_types();
  Optional<TypeHandle> lookup_function(const types::Abstraction& func) const;
  bool is_known_subscript_type(const TypeHandle& handle) const;

private:
  void make_binary_operators();
  void make_subscript_references();
  void make_concatenations();
  void make_builtin_parens_subscript_references();
  void make_builtin_brace_subscript_reference();
  void make_free_functions();
  void make_min();
  void make_fileparts();
  void make_list_outputs_type();
  void make_list_outputs_type2();
  void make_list_inputs_type();

private:
  TypeStore& store;
  StringRegistry& string_registry;

  TypeEquality::ArgumentComparator arg_comparator;
  TypeEquality::TypeEquivalenceComparator type_equiv_comparator;

  std::map<types::Abstraction, TypeHandle, TypeEquality::ArgumentComparator> function_types;
  std::set<TypeHandle, TypeEquality::TypeEquivalenceComparator> types_with_known_subscripts;
};

}