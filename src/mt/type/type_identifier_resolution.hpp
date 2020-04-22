#pragma once

#include "../ast/visitor.hpp"
#include "../traversal.hpp"
#include "../identifier.hpp"
#include "../error.hpp"
#include "../source_data.hpp"
#include <vector>

namespace mt {

struct TypeScope;
struct MatlabScope;
class Store;
class TypeStore;
class Library;
struct Type;
class StringRegistry;

namespace types {
  struct Scheme;
  struct Alias;
}

class TypeIdentifierResolverInstance {
public:
  friend class TypeIdentifierResolver;

  struct TypeCollector {
    TypeCollector();
    void push(Type* type);
    void mark_error();

    std::vector<Type*> types;
    bool had_error;
  };

  struct TypeCollectorState {
    void push();
    void pop();
    TypeCollector& current();
    TypeCollector& parent();

    std::vector<TypeCollector> type_collectors;
  };

  struct UnresolvedIdentifier {
    UnresolvedIdentifier(const TypeIdentifier& ident, TypeScope* scope);

    TypeIdentifier identifier;
    TypeScope* scope;
  };

  struct PendingScheme {
    PendingScheme(const types::Scheme* scheme, std::vector<Type*> arguments, types::Alias* target);
    void instantiate(TypeStore& store) const;

    MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(PendingScheme)

    const types::Scheme* scheme;
    std::vector<Type*> arguments;
    types::Alias* target;
  };

  struct SchemeVariables {
    std::unordered_map<TypeIdentifier, Type*, TypeIdentifier::Hash> variables;
  };
public:
  TypeIdentifierResolverInstance(TypeStore& type_store, Library& library, const Store& def_store,
                                 const StringRegistry& string_registry, const TokenSourceMap& source_map);

  bool had_error() const;
  void add_error(const ParseError& err);
  void add_unresolved_identifier(const TypeIdentifier& ident, TypeScope* in_scope);
  void mark_parent_collector_error();
  void push_scheme();
  void pop_scheme();
  bool has_scheme() const;
  SchemeVariables& current_scheme_variables();

  void push_pending_scheme(PendingScheme&& pending_scheme);

public:
  TypeStore& type_store;
  Library& library;
  const Store& def_store;
  const StringRegistry& string_registry;
  const TokenSourceMap& source_map;

  ParseErrors errors;
  std::vector<UnresolvedIdentifier> unresolved_identifiers;
  std::vector<PendingScheme> pending_schemes;

  TypeIdentifierExportState export_state;
  TypeIdentifierNamespaceState namespace_state;
  ScopeState<TypeScope> scopes;
  ScopeState<const MatlabScope> matlab_scopes;
  TypeCollectorState collectors;
  BooleanState polymorphic_function_state;
  std::vector<SchemeVariables> scheme_variables;
};

class TypeIdentifierResolver : public TypePreservingVisitor {
public:
  explicit TypeIdentifierResolver(TypeIdentifierResolverInstance* instance);

  void root_block(RootBlock& block) override;
  void block(Block& block) override;

  void type_annot_macro(TypeAnnotMacro& macro) override;
  void inline_type(InlineType& node) override;
  void type_begin(TypeBegin& begin) override;
  void type_assertion(TypeAssertion& node) override;
  void type_given(TypeGiven& node) override;

  void fun_type_node(FunTypeNode& node) override;
  void function_type_node(FunctionTypeNode& node) override;
  void scalar_type_node(ScalarTypeNode& node) override;
  void tuple_type_node(TupleTypeNode& node) override;
  void list_type_node(ListTypeNode& node) override;
  void record_type_node(RecordTypeNode& node) override;
  void declare_type_node(DeclareTypeNode& node) override;
  void namespace_type_node(NamespaceTypeNode& node) override;
  void infer_type_node(InferTypeNode& node) override;

  void type_let(TypeLet& node) override;

  void function_def_node(FunctionDefNode& node) override;
  void class_def_node(ClassDefNode& node) override;
  void property_node(PropertyNode& node) override;
  void method_node(MethodNode& node) override;

private:
  void scalar_type_declaration(DeclareTypeNode& node);
  void method_type_declaration(DeclareTypeNode& node);

  Type* resolve_identifier_reference(ScalarTypeNode& node) const;
  Type* handle_pending_scheme(const types::Scheme* scheme, ScalarTypeNode& node);

private:
  TypeIdentifierResolverInstance* instance;
};

}