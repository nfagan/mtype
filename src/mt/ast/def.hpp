#pragma once

#include "ast.hpp"
#include "../handles.hpp"
#include "../token.hpp"
#include "../identifier.hpp"
#include "../lang_components.hpp"
#include "../utility.hpp"

namespace mt {

class TypePreservingVisitor;
struct TypeScope;
struct Type;
struct TypeNode;

struct FunctionDefNode : public AstNode {
  FunctionDefNode(const Token& source_token,
                  const FunctionDefHandle& def_handle,
                  const FunctionReferenceHandle& ref_handle,
                  MatlabScope* scope,
                  TypeScope* type_scope) :
    source_token(source_token), def_handle(def_handle), ref_handle(ref_handle), scope(scope),
    type_scope(type_scope) {
    //
  }
  ~FunctionDefNode() override = default;

  bool is_function_def_node() const override {
    return true;
  }

  std::string accept(const StringVisitor& vis) const override;
  FunctionDefNode* accept(IdentifierClassifier& classifier) override;
  void accept(TypePreservingVisitor& vis) override;
  void accept_const(TypePreservingVisitor& vis) const override;

  Optional<FunctionDefNode*> extract_function_def_node() override;

  Token source_token;
  FunctionDefHandle def_handle;
  FunctionReferenceHandle ref_handle;
  MatlabScope* scope;
  TypeScope* type_scope;
};

using BoxedFunctionDefNode = std::unique_ptr<FunctionDefNode>;
using BoxedFunctionDefNodes = std::vector<BoxedFunctionDefNode>;

struct PropertyNode : public AstNode {
  PropertyNode(const Token& source_token, const MatlabIdentifier& name, BoxedExpr initializer, BoxedTypeAnnot type) :
    source_token(source_token), name(name), initializer(std::move(initializer)), type(std::move(type)) {
    //
  }

  ~PropertyNode() override = default;

  std::string accept(const StringVisitor& vis) const override;
  PropertyNode* accept(IdentifierClassifier& classifier) override;
  void accept(TypePreservingVisitor& vis) override;
  void accept_const(TypePreservingVisitor& vis) const override;

  bool is_property() const override {
    return true;
  }

  Token source_token;
  MatlabIdentifier name;
  BoxedExpr initializer;
  BoxedTypeAnnot type;
};

struct MethodNode : public AstNode {
  MethodNode(BoxedFunctionDefNode def, BoxedTypeAnnot type, Type* resolved_type) :
  def(std::move(def)), type(std::move(type)), resolved_type(resolved_type) {
    //
  }
  ~MethodNode() override = default;

  std::string accept(const StringVisitor& vis) const override;
  MethodNode* accept(IdentifierClassifier& classifier) override;
  void accept(TypePreservingVisitor& vis) override;
  void accept_const(TypePreservingVisitor& vis) const override;

  BoxedFunctionDefNode def;
  BoxedTypeAnnot type;
  BoxedBlock external_block;
  Type* resolved_type;
};

using BoxedPropertyNode = std::unique_ptr<PropertyNode>;
using BoxedPropertyNodes = std::vector<BoxedPropertyNode>;
using BoxedMethodNode = std::unique_ptr<MethodNode>;
using BoxedMethodNodes = std::vector<BoxedMethodNode>;

struct ClassDefNode : public AstNode {
public:
  struct MethodDeclaration {
    MethodDeclaration(const FunctionDefHandle& def_handle, std::unique_ptr<TypeNode> type_node) :
    pending_def_handle(def_handle), maybe_type(std::move(type_node)) {
      //
    }

    FunctionDefHandle pending_def_handle;
    std::unique_ptr<TypeNode> maybe_type;
  };
  using MethodDeclarations = std::vector<MethodDeclaration>;

public:
  ClassDefNode(const Token& source_token,
               const ClassDefHandle& handle,
               BoxedPropertyNodes&& properties,
               BoxedMethodNodes&& method_defs) :
               source_token(source_token),
               handle(handle),
               properties(std::move(properties)),
               method_defs(std::move(method_defs)) {
    //
  }
  ~ClassDefNode() override = default;

  bool is_class_def_node() const override {
    return true;
  }

  std::string accept(const StringVisitor& vis) const override;
  ClassDefNode* accept(IdentifierClassifier& classifier) override;
  void accept(TypePreservingVisitor& vis) override;
  void accept_const(TypePreservingVisitor& vis) const override;

  Token source_token;
  ClassDefHandle handle;
  BoxedPropertyNodes properties;
  BoxedMethodNodes method_defs;
};


}