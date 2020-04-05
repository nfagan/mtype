#pragma once

#include "type.hpp"
#include "type_relation.hpp"
#include "type_relationships.hpp"
#include "../Optional.hpp"
#include "../handles.hpp"
#include "../store.hpp"
#include "../search_path.hpp"
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mt {

class TypeStore;
class StringRegistry;
class FunctionDefHandle;
class SearchPath;

/*
 * MethodStore
 */

class MethodStore {
  using TypedMethods = std::map<types::Abstraction, Type*, types::Abstraction::HeaderLess>;
  using MethodMap = std::unordered_map<const types::Class*, TypedMethods>;

public:
  MethodStore() = default;

  Optional<Type*> lookup_method(const types::Class* cls, const types::Abstraction& by_header) const;
  bool has_method(const types::Class* cls, const types::Abstraction& method) const;
  void add_method(const types::Class* to_class, const types::Abstraction& ref, Type* type);

private:
  TypedMethods& require_methods(const types::Class* for_class);

private:
  MethodMap methods;
};

/*
 * Library
 */

class Library {
  friend class Unifier;
public:
  struct FunctionSearchResult {
    FunctionSearchResult(Optional<Type*> type) : resolved_type(std::move(type)) {
      //
    }

    FunctionSearchResult(Optional<const SearchPath::Candidate*> candidate) :
      external_function_candidate(std::move(candidate)) {
      //
    }

    Optional<Type*> resolved_type;
    Optional<const SearchPath::Candidate*> external_function_candidate;
  };

public:
  explicit Library(TypeStore& store, const Store& def_store, const SearchPath& search_path, StringRegistry& string_registry) :
  subtype_relation(*this),
  type_eq(equiv_relation, store),
  store(store),
  def_store(def_store),
  string_registry(string_registry),
  arg_comparator(type_eq),
  name_comparator(type_eq),
  function_types(name_comparator),
  search_path(search_path) {
    //
  }

  void make_known_types();
  MT_NODISCARD Optional<Type*> lookup_function(const types::Abstraction& func) const;
  MT_NODISCARD FunctionSearchResult search_function(const types::Abstraction& func) const;
  bool is_known_subscript_type(const Type* type) const;

  MT_NODISCARD Optional<std::string> type_name(const Type* type) const;
  MT_NODISCARD Optional<std::string> type_name(const types::Scalar& scl) const;

  void emplace_local_function_type(const FunctionDefHandle& handle, Type* type);

  //  Test a <: b
  bool subtype_related(const Type* lhs, const Type* rhs) const;
  bool subtype_related(const Type* lhs, const Type* rhs, const types::Scalar& a, const types::Scalar& b) const;

private:
  void make_builtin_types();
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
  void make_double_methods();

  Optional<types::Class*> class_wrapper(const Type* type) const;
  Optional<const types::Class*> class_for_type(const Type* type) const;

  MT_NODISCARD Optional<Type*> lookup_local_function(const FunctionDefHandle& def_handle) const;
  MT_NODISCARD Optional<Type*> lookup_pre_defined_external_function(const types::Abstraction& func) const;
  MT_NODISCARD Optional<Type*> method_dispatch(const types::Abstraction& func) const;
  MT_NODISCARD FunctionDefHandle maybe_extract_function_def(const types::Abstraction& func) const;

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
  std::unordered_map<const Type*, types::Class*> class_wrappers;

  MethodStore method_store;
  const SearchPath& search_path;

public:
  Type* double_type_handle;
  Type* double_type_class;
  Type* char_type_handle;
  Type* string_type_handle;
  Type* logical_type_handle;
  Type* sub_double_type_handle;
  Type* sub_sub_double_type_handle;
};

}