#pragma once

#include "../ast/visitor.hpp"
#include "../traversal.hpp"
#include "../identifier.hpp"
#include "error.hpp"
#include <vector>

namespace mt {

struct TypeScope;
class Store;
class TypeStore;

class TypeIdentifierResolverInstance {
  struct UnresolvedIdentifier {
    UnresolvedIdentifier(const TypeIdentifier& ident, TypeScope* scope);

    TypeIdentifier identifier;
    TypeScope* scope;
  };

  friend class TypeIdentifierResolver;
public:
  TypeIdentifierResolverInstance(TypeStore& type_store, const Store& def_store);

  bool had_error() const;
  void add_error(BoxedTypeError err);
  void add_unresolved_identifier(const TypeIdentifier& ident, TypeScope* in_scope);

public:
  TypeStore& type_store;
  const Store& def_store;

  std::vector<BoxedTypeError> errors;
  std::vector<UnresolvedIdentifier> unresolved_identifiers;
  TypeIdentifierExportState export_state;
  TypeIdentifierNamespaceState namespace_state;
  ScopeState<TypeScope> scopes;
};

class TypeIdentifierResolver : public TypePreservingVisitor {
public:
  TypeIdentifierResolver(TypeIdentifierResolverInstance* instance);

  void root_block(const RootBlock& block) override;
  void block(const Block& block) override;

  void type_annot_macro(const TypeAnnotMacro& macro) override;
  void inline_type(const InlineType& node) override;
  void type_begin(const TypeBegin& begin) override;

  void function_type_node(const FunctionTypeNode& node) override;
  void scalar_type_node(const ScalarTypeNode& node) override;
  void record_type_node(const RecordTypeNode& node) override;
  void declare_type_node(const DeclareTypeNode& node) override;
  void namespace_type_node(const NamespaceTypeNode& node) override;

  void type_let(const TypeLet& node) override;

  void function_def_node(const FunctionDefNode& node) override;
  void class_def_node(const ClassDefNode& node) override;

private:
  TypeIdentifierResolverInstance* instance;
};

}