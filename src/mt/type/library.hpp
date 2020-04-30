#pragma once

#include "type.hpp"
#include "type_relation.hpp"
#include "type_relationships.hpp"
#include "pending_external_functions.hpp"
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
  int64_t varargin;
  int64_t varargout;
  int64_t isa;
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
  using LocalFunctionTypes =
    std::unordered_map<FunctionDefHandle, Type*, FunctionDefHandle::Hash>;
  using LocalClassTypes =
    std::unordered_map<ClassDefHandle, types::Class*, ClassDefHandle::Hash>;
  using LocalVariableTypes =
    std::unordered_map<VariableDefHandle, Type*, VariableDefHandle::Hash>;

  Library(TypeStore& store, Store& def_store, const SearchPath& search_path,
          StringRegistry& string_registry);

  types::Scalar* make_named_scalar_type(const TypeIdentifier& name);

  MT_NODISCARD Optional<Type*> lookup_function(const types::Abstraction& func) const;
  MT_NODISCARD Optional<Type*> lookup_local_function(const FunctionDefHandle& def_handle) const;
  MT_NODISCARD Optional<Type*> lookup_method(const Type* type, const types::Abstraction& func) const;
  MT_NODISCARD FunctionSearchResult search_function(const types::Abstraction& func,
                                                    const TypePtrs& args) const;

  MT_NODISCARD Optional<types::Class*> lookup_class(const TypeIdentifier& name) const;
  MT_NODISCARD Optional<types::Class*> lookup_local_class(const ClassDefHandle& def_handle) const;
  MT_NODISCARD Optional<Type*> lookup_local_variable(const VariableDefHandle& def_handle) const;
  MT_NODISCARD Optional<const types::Class*> class_for_type(const Type* type) const;

  bool is_builtin_class(const MatlabIdentifier& ident) const;

  bool has_declared_function_type(const TypeIdentifier& ident) const;
  void register_declared_function(const TypeIdentifier& ident);
  bool emplace_declared_function_type(const types::Abstraction& abstr, Type* source);

  MT_NODISCARD Optional<std::string> type_name(const Type* type) const;

  void emplace_local_function_type(const FunctionDefHandle& handle, Type* type);
  void replace_local_function_type(const FunctionDefHandle& handle, Type* type);
  bool emplace_local_class_type(const ClassDefHandle& handle, types::Class* type);
  void emplace_local_variable_type(const VariableDefHandle& handle, Type* type);
  Type* require_local_variable_type(const VariableDefHandle& handle);

  //  Test a <: b
  bool subtype_related(const Type* lhs, const Type* rhs) const;

  MT_NODISCARD Optional<Type*> get_number_type() const;
  MT_NODISCARD Optional<Type*> get_sub_number_type() const;
  MT_NODISCARD Optional<Type*> get_char_type() const;
  MT_NODISCARD Optional<Type*> get_string_type() const;
  MT_NODISCARD Optional<Type*> get_logical_type() const;

  const LocalFunctionTypes& get_local_function_types() const;

private:
  void make_known_types();
  void make_base_type_scope();

  void make_builtin_types();
  void make_concatenations();
  void make_free_functions();
  void make_feval();
  void make_deal();
  void make_subtype_debug();

  Optional<types::Class*> class_wrapper(const Type* type) const;

  MT_NODISCARD Optional<FunctionSearchResult>
  search_function(const FunctionReferenceHandle& ref_handle) const;

  MT_NODISCARD Optional<Type*>
  lookup_pre_defined_external_function(const types::Abstraction& func) const;

  MT_NODISCARD Optional<Type*>
  method_dispatch(const types::Abstraction& func, const TypePtrs& args) const;

  MT_NODISCARD FunctionDefHandle maybe_extract_function_def(const types::Abstraction& func) const;

  types::Abstraction* make_simple_function(const char* name, TypePtrs&& args, TypePtrs&& outs);
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

  std::map<types::Abstraction, Type*, types::Abstraction::HeaderLess> function_types;

  LocalFunctionTypes local_function_types;
  LocalClassTypes local_class_types;
  LocalVariableTypes local_variables_types;

  std::unordered_map<TypeIdentifier, types::Class*, TypeIdentifier::Hash> class_types;
  std::unordered_set<TypeIdentifier, TypeIdentifier::Hash> declared_function_types;

  const SearchPath& search_path;

  TypeIdentifier double_id;
  TypeIdentifier sub_double_id;
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