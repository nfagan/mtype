#pragma once

#include "type.hpp"
#include "type_relation.hpp"
#include "type_relationships.hpp"
#include "../Optional.hpp"
#include "../handles.hpp"
#include "../store.hpp"
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mt {

class TypeStore;
class StringRegistry;
class FunctionDefHandle;
class SearchPath;
struct SearchCandidate;
struct TypeScope;

/*
 * SpecialIdentifierStore
 */

struct SpecialIdentifierStore {
  explicit SpecialIdentifierStore(StringRegistry& string_registry);
  int64_t subsref;
  int64_t subsindex;
  int64_t identifier_struct;
  int64_t handle;
};

/*
 * ScalarTypeStore
 */

class ScalarTypeStore {
public:
  ScalarTypeStore(TypeStore& store, StringRegistry& string_registry) :
  store(store), string_registry(string_registry) {
    //
  }

  bool contains(const TypeIdentifier& name) const;
  types::Scalar* make_named_scalar_type(const char* name);
  types::Scalar* make_named_scalar_type(const TypeIdentifier& name);
  Optional<Type*> lookup(const TypeIdentifier& name) const;

private:
  TypeStore& store;
  StringRegistry& string_registry;

  std::unordered_map<TypeIdentifier, Type*, TypeIdentifier::Hash> scalar_types;
};

/*
 * MethodStore
 */

class MethodStore {
  using TypedMethods = std::map<types::Abstraction, Type*, types::Abstraction::HeaderLess>;
  using MethodsByClass = std::unordered_map<const types::Class*, TypedMethods>;

public:
  MethodStore() = default;

  Optional<Type*> lookup_method(const types::Class* cls, const types::Abstraction& by_header) const;
  void add_method(const types::Class* to_class, const types::Abstraction& ref, Type* type);

  bool has_method(const types::Class* cls, const types::Abstraction& method) const;
  bool has_named_method(const types::Class* cls, const MatlabIdentifier& name) const;

private:
  TypedMethods& require_methods(const types::Class* for_class);

private:
  MethodsByClass methods;
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

    FunctionSearchResult(Optional<const SearchCandidate*> candidate) :
      external_function_candidate(std::move(candidate)) {
      //
    }

    Optional<Type*> resolved_type;
    Optional<const SearchCandidate*> external_function_candidate;
  };

public:
  explicit Library(TypeStore& store, Store& def_store, const SearchPath& search_path, StringRegistry& string_registry) :
  subtype_relation(*this),
  type_eq(equiv_relation, store),
  store(store),
  def_store(def_store),
  string_registry(string_registry),
  arg_comparator(type_eq),
  name_comparator(type_eq),
  function_types(name_comparator),
  search_path(search_path),
  scalar_store(store, string_registry),
  special_identifiers(string_registry) {
    //
  }

  void make_known_types();
  void make_base_type_scope();
  types::Scalar* make_named_scalar_type(const TypeIdentifier& name);

  MT_NODISCARD Optional<Type*> lookup_function(const types::Abstraction& func) const;
  MT_NODISCARD Optional<Type*> lookup_local_function(const FunctionDefHandle& def_handle) const;
  MT_NODISCARD Optional<Type*> lookup_method(const Type* type, const types::Abstraction& func) const;
  MT_NODISCARD FunctionSearchResult search_function(const types::Abstraction& func) const;

  MT_NODISCARD Optional<types::Class*> lookup_class(const TypeIdentifier& name) const;
  MT_NODISCARD Optional<types::Class*> lookup_local_class(const ClassDefHandle& def_handle) const;
  MT_NODISCARD Optional<Type*> lookup_local_variable(const VariableDefHandle& def_handle) const;
  MT_NODISCARD Optional<const types::Class*> class_for_type(const Type* type) const;

  bool is_known_subscript_type(const Type* type) const;
  bool is_builtin_class(const MatlabIdentifier& ident) const;

  MT_NODISCARD Optional<std::string> type_name(const Type* type) const;

  void emplace_local_function_type(const FunctionDefHandle& handle, Type* type);
  void emplace_local_class_type(const ClassDefHandle& handle, types::Class* type);
  Type* require_local_variable_type(const VariableDefHandle& handle);

  //  Test a <: b
  bool subtype_related(const Type* lhs, const Type* rhs) const;
//  bool subtype_related(const Type* lhs, const Type* rhs, const types::Scalar& a, const types::Scalar& b) const;

  Optional<Type*> get_number_type() const;
  Optional<Type*> get_char_type() const;
  Optional<Type*> get_string_type() const;
  Optional<Type*> get_logical_type() const;

private:
  void make_builtin_types();
  void make_subscript_references();
  void make_concatenations();
  void make_builtin_parens_subscript_references();
  void make_builtin_brace_subscript_reference();
  void make_free_functions();
  void make_sum();
  void make_feval();
  void make_deal();
  void make_logicals();

  Optional<types::Class*> class_wrapper(const Type* type) const;

  MT_NODISCARD Optional<Type*> lookup_pre_defined_external_function(const types::Abstraction& func) const;
  MT_NODISCARD Optional<Type*> method_dispatch(const types::Abstraction& func, const TypePtrs& args) const;
  MT_NODISCARD FunctionDefHandle maybe_extract_function_def(const types::Abstraction& func) const;

  void add_type_with_known_subscript(const Type* t);

  Type* make_simple_function(const char* name, TypePtrs&& args, TypePtrs&& outs);
  types::Class* make_class_wrapper(const TypeIdentifier& name, Type* source);
  types::Scalar* make_named_scalar_type(const char* name);
  void process_scalar_type(types::Scalar* type);
  void add_type_to_base_scope(const TypeIdentifier& ident, Type* type);

private:
  SubtypeRelation subtype_relation;
  EquivalenceRelation equiv_relation;
  TypeRelation type_eq;

  TypeStore& store;
  Store& def_store;
  StringRegistry& string_registry;

  TypeRelation::ArgumentLess arg_comparator;
  TypeRelation::NameLess name_comparator;

  std::map<types::Abstraction, Type*, TypeRelation::NameLess> function_types;
  std::vector<const Type*> types_with_known_subscripts;

  std::unordered_map<FunctionDefHandle, Type*, FunctionDefHandle::Hash> local_function_types;
  std::unordered_map<ClassDefHandle, types::Class*, ClassDefHandle::Hash> local_class_types;
  std::unordered_map<VariableDefHandle, Type*, VariableDefHandle::Hash> local_variables_types;

  std::unordered_map<TypeIdentifier, types::Class*, TypeIdentifier::Hash> class_types;

  const SearchPath& search_path;

  TypeIdentifier double_id;
  TypeIdentifier char_id;
  TypeIdentifier string_id;
  TypeIdentifier logical_id;

public:
  MethodStore method_store;
  ScalarTypeStore scalar_store;
  SpecialIdentifierStore special_identifiers;

  TypeScope* base_scope;
};

}