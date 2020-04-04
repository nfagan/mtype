#pragma once

#include "type.hpp"
#include "type_relation.hpp"
#include "type_relationships.hpp"
#include "../Optional.hpp"
#include "../handles.hpp"
#include "../store.hpp"
#include <map>
#include <unordered_map>
#include <vector>

namespace mt {

class TypeStore;
class StringRegistry;
class FunctionDefHandle;

class Library {
  friend class Unifier;
public:
  explicit Library(TypeStore& store, const Store& def_store, StringRegistry& string_registry) :
  subtype_relation(*this),
  type_eq(equiv_relation, store),
  store(store),
  def_store(def_store),
  string_registry(string_registry),
  arg_comparator(type_eq),
  name_comparator(type_eq),
  function_types(name_comparator) {
    //
  }

  void make_known_types();
  Optional<Type*> lookup_function(const types::Abstraction& func) const;
  bool is_known_subscript_type(const Type* type) const;

  Optional<std::string> type_name(const Type* type) const;
  Optional<std::string> type_name(const types::Scalar& scl) const;

  void emplace_local_function_type(const FunctionDefHandle& handle, Type* type);

  //  Test a <: b
  bool subtype_related(const Type* lhs, const Type* rhs) const;
  bool subtype_related(const Type* lhs, const Type* rhs, const types::Scalar& a, const types::Scalar& b) const;

private:
  void make_builtin_types();
  void make_binary_operators();
  void make_subscript_references();
  void make_concatenations();
  void make_builtin_parens_subscript_references();
  void make_builtin_brace_subscript_reference();
  void make_function_as_input();
  void make_free_functions();
  void make_min();
  void make_sum();
  void make_feval();
  void make_deal();
  void make_logicals();
  void make_list_outputs_type();
  void make_list_outputs_type2();
  void make_list_inputs_type();
  void make_sub_sub_double();
  void make_sub_double();
  void make_double();

  Optional<Type*> lookup_local_function(const FunctionDefHandle& def_handle) const;

  void add_type_with_known_subscript(const Type* t);

  Type* make_simple_function(const char* name, TypePtrs&& args, TypePtrs&& outs);
  Type* make_named_scalar_type(const char* name);

private:
  SubtypeRelation subtype_relation;
  EquivalenceRelation equiv_relation;
  TypeRelation type_eq;

  TypeStore& store;
  const Store& def_store;
  StringRegistry& string_registry;

  TypeRelation::ArgumentLess arg_comparator;
  TypeRelation::NameLess name_comparator;

  std::map<types::Abstraction, Type*, TypeRelation::NameLess> function_types;
  std::vector<const Type*> types_with_known_subscripts;
  std::unordered_map<FunctionDefHandle, Type*, FunctionDefHandle::Hash> local_function_types;

  std::unordered_map<TypeIdentifier, int64_t, TypeIdentifier::Hash> scalar_type_names;
  std::unordered_map<const Type*, types::Class> scalar_subtype_relations;

public:
  Type* double_type_handle;
  Type* char_type_handle;
  Type* string_type_handle;
  Type* logical_type_handle;
  Type* sub_double_type_handle;
  Type* sub_sub_double_type_handle;
};

}