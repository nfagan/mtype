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

  bool represents_function_def() const override {
    return true;
  }

  std::string accept(const StringVisitor& vis) const override;
  FunctionDefNode* accept(IdentifierClassifier& classifier) override;
  void accept(TypePreservingVisitor& vis) override;
  void accept_const(TypePreservingVisitor& vis) const override;

  Token source_token;
  FunctionDefHandle def_handle;
  FunctionReferenceHandle ref_handle;
  MatlabScope* scope;
  TypeScope* type_scope;
};

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

using BoxedPropertyNode = std::unique_ptr<PropertyNode>;
using BoxedPropertyNodes = std::vector<BoxedPropertyNode>;

struct ClassDefNode : public AstNode {
public:
  ClassDefNode(const Token& source_token,
               const ClassDefHandle& handle,
               BoxedPropertyNodes&& properties,
               std::vector<std::unique_ptr<FunctionDefNode>>&& method_defs) :
               source_token(source_token),
               handle(handle),
               properties(std::move(properties)),
               method_defs(std::move(method_defs)) {
    //
  }
  ~ClassDefNode() override = default;

  bool represents_class_def() const override {
    return true;
  }

  std::string accept(const StringVisitor& vis) const override;
  ClassDefNode* accept(IdentifierClassifier& classifier) override;
  void accept(TypePreservingVisitor& vis) override;
  void accept_const(TypePreservingVisitor& vis) const override;

  Token source_token;
  ClassDefHandle handle;
  BoxedPropertyNodes properties;
  std::vector<std::unique_ptr<FunctionDefNode>> method_defs;
};


}