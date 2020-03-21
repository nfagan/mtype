#pragma once

#include "ast.hpp"
#include "../handles.hpp"
#include "../token.hpp"
#include "../lang_components.hpp"
#include "../utility.hpp"

namespace mt {

class TypePreservingVisitor;

struct FunctionDefNode : public AstNode {
  FunctionDefNode(const Token& source_token, const FunctionDefHandle& def_handle, const MatlabScopeHandle& scope_handle) :
  source_token(source_token), def_handle(def_handle), scope_handle(scope_handle) {
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
  MatlabScopeHandle scope_handle;
};

struct ClassDefNode : public AstNode {
public:
  struct Property {
    Property() = default;
    Property(const Token& source_token, int64_t name, BoxedExpr initializer) :
      source_token(source_token), name(name), initializer(std::move(initializer)) {
      //
    }
    Property(Property&& other) MSVC_MISSING_NOEXCEPT = default;
    Property& operator=(Property&& other) MSVC_MISSING_NOEXCEPT = default;
    ~Property() = default;

    Token source_token;
    int64_t name;
    BoxedExpr initializer;
  };

public:
  ClassDefNode(const Token& source_token,
               const ClassDefHandle& handle,
               std::vector<Property>&& properties,
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
  std::vector<Property> properties;
  std::vector<std::unique_ptr<FunctionDefNode>> method_defs;
};


}