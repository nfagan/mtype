#pragma once

#include "../ast/visitor.hpp"
#include "../traversal.hpp"
#include "../identifier.hpp"
#include "../error.hpp"
#include "../source_data.hpp"
#include <vector>

namespace mt {

struct TypeScope;
class Store;
class TypeStore;
class Library;
struct Type;
class StringRegistry;

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
    struct Stack {
      static void push(TypeCollectorState& collector);
      static void pop(TypeCollectorState& collector);
    };
    using Helper = StackHelper<TypeCollectorState, Stack>;

    void push();
    void pop();
    TypeCollector& current();

    std::vector<TypeCollector> type_collectors;
  };

  struct UnresolvedIdentifier {
    UnresolvedIdentifier(const TypeIdentifier& ident, TypeScope* scope);

    TypeIdentifier identifier;
    TypeScope* scope;
  };
public:
  TypeIdentifierResolverInstance(TypeStore& type_store, Library& library, const Store& def_store,
                                 const StringRegistry& string_registry, const ParseSourceData& source_data);

  bool had_error() const;
  void add_error(const ParseError& err);
  void add_unresolved_identifier(const TypeIdentifier& ident, TypeScope* in_scope);

public:
  TypeStore& type_store;
  Library& library;
  const Store& def_store;
  const StringRegistry& string_registry;
  const ParseSourceData& source_data;

  ParseErrors errors;
  std::vector<UnresolvedIdentifier> unresolved_identifiers;
  TypeIdentifierExportState export_state;
  TypeIdentifierNamespaceState namespace_state;
  ScopeState<TypeScope> scopes;
  TypeCollectorState collectors;
};

class TypeIdentifierResolver : public TypePreservingVisitor {
public:
  TypeIdentifierResolver(TypeIdentifierResolverInstance* instance);

  void root_block(const RootBlock& block) override;
  void block(const Block& block) override;

  void type_annot_macro(const TypeAnnotMacro& macro) override;
  void inline_type(const InlineType& node) override;
  void type_begin(const TypeBegin& begin) override;
  void type_assertion(const TypeAssertion& node) override;

  void function_type_node(const FunctionTypeNode& node) override;
  void scalar_type_node(const ScalarTypeNode& node) override;
  void record_type_node(const RecordTypeNode& node) override;
  void declare_type_node(const DeclareTypeNode& node) override;
  void namespace_type_node(const NamespaceTypeNode& node) override;

  void type_let(const TypeLet& node) override;

  void function_def_node(const FunctionDefNode& node) override;
  void class_def_node(const ClassDefNode& node) override;

private:
  void scalar_type_declaration(const DeclareTypeNode& node);
  void method_type_declaration(const DeclareTypeNode& node);

private:
  TypeIdentifierResolverInstance* instance;
};

}