#pragma once

#include "type.hpp"
#include "type_relation.hpp"
#include "type_relationships.hpp"
#include <map>

namespace mt {

class TypeStore;

class Library {
  friend class Unifier;
public:
  explicit Library(TypeStore& store, StringRegistry& string_registry) :
  subtype_relation(*this),
  type_eq(equiv_relation, store),
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

  Optional<std::string> type_name(const TypeHandle& type) const;
  Optional<std::string> type_name(const types::Scalar& scl) const;

  //  Test a <: b
  bool subtype_related(TypeRef lhs, TypeRef rhs) const;
  bool subtype_related(TypeRef lhs, TypeRef rhs, const types::Scalar& a, const types::Scalar& b) const;

private:
  void make_builtin_types();
  void make_binary_operators();
  void make_subscript_references();
  void make_concatenations();
  void make_builtin_parens_subscript_references();
  void make_builtin_brace_subscript_reference();
  void make_builtin_parens_tuple_subscript_reference();
  void make_function_as_input();
  void make_free_functions();
  void make_min();
  void make_sum();
  void make_fileparts();
  void make_list_outputs_type();
  void make_list_outputs_type2();
  void make_list_inputs_type();
  void make_sub_double();
  void make_double();

  TypeHandle make_named_scalar_type(const char* name);

private:
  SubtypeRelation subtype_relation;
  EquivalenceRelation equiv_relation;
  TypeRelation type_eq;

  TypeStore& store;
  StringRegistry& string_registry;

  TypeRelation::ArgumentComparator arg_comparator;
  TypeRelation::TypeRelationComparator type_equiv_comparator;

  std::map<types::Abstraction, TypeHandle, TypeRelation::ArgumentComparator> function_types;
  std::set<TypeHandle, TypeRelation::TypeRelationComparator> types_with_known_subscripts;

  std::unordered_map<TypeIdentifier, int64_t, TypeIdentifier::Hash> scalar_type_names;
  std::unordered_map<TypeHandle, types::SubtypeRelation, TypeHandle::Hash> scalar_subtype_relations;

public:
  TypeHandle double_type_handle;
  TypeHandle char_type_handle;
  TypeHandle string_type_handle;
  TypeHandle sub_double_type_handle;
};

}