#pragma once

#include "ast.hpp"
#include "../token.hpp"

namespace mt {

struct TypeAnnotMacro : public TypeAnnot {
  TypeAnnotMacro(const Token& source_token, BoxedTypeAnnot annotation) :
  source_token(source_token), annotation(std::move(annotation)) {
    //
  }
  ~TypeAnnotMacro() override = default;

  std::string accept(const StringVisitor& vis) const override;
  TypeAnnotMacro* accept(IdentifierClassifier& classifier) override;
  void accept_const(TypePreservingVisitor& vis) const override;
  void accept(TypePreservingVisitor& vis) override;

  Token source_token;
  BoxedTypeAnnot annotation;
};

struct TypeAssertion : public TypeAnnot {
  TypeAssertion(const Token& source_token, BoxedAstNode node, BoxedType has_type) :
  source_token(source_token), node(std::move(node)), has_type(std::move(has_type)) {
    //
  }

  ~TypeAssertion() override = default;

  std::string accept(const StringVisitor& vis) const override;
  TypeAssertion* accept(IdentifierClassifier& classifier) override;
  void accept_const(TypePreservingVisitor& vis) const override;
  void accept(TypePreservingVisitor& vis) override;

  Token source_token;
  BoxedAstNode node;
  BoxedType has_type;
};

struct InlineType : public TypeAnnot {
  explicit InlineType(BoxedType type) : type(std::move(type)) {
    //
  }
  ~InlineType() override = default;

  std::string accept(const StringVisitor& vis) const override;
  void accept_const(TypePreservingVisitor& vis) const override;
  void accept(TypePreservingVisitor& vis) override;

  BoxedType type;
};

struct TypeLet : public TypeAnnot {
  TypeLet(const Token& source_token,
          int64_t identifier,
          BoxedType equal_to_type) :
          source_token(source_token),
          identifier(identifier),
          equal_to_type(std::move(equal_to_type)) {
    //
  }
  ~TypeLet() override = default;

  std::string accept(const StringVisitor& vis) const override;
  void accept_const(TypePreservingVisitor& vis) const override;
  void accept(TypePreservingVisitor& vis) override;

  Token source_token;
  int64_t identifier;
  BoxedType equal_to_type;
};

struct TypeGiven : public TypeAnnot {
  TypeGiven(const Token& source_token,
            std::vector<int64_t>&& identifiers,
            BoxedTypeAnnot declaration) :
            source_token(source_token),
            identifiers(std::move(identifiers)),
            declaration(std::move(declaration)) {
    //
  }
  ~TypeGiven() override = default;

  std::string accept(const StringVisitor& vis) const override;
  void accept_const(TypePreservingVisitor& vis) const override;
  void accept(TypePreservingVisitor& vis) override;

  Token source_token;
  std::vector<int64_t> identifiers;
  BoxedTypeAnnot declaration;
};

struct TypeBegin : public TypeAnnot {
  TypeBegin(const Token& source_token, bool is_exported, std::vector<BoxedTypeAnnot>&& contents) :
  source_token(source_token), is_exported(is_exported), contents(std::move(contents)) {
    //
  }
  ~TypeBegin() override = default;

  std::string accept(const StringVisitor& vis) const override;
  void accept_const(TypePreservingVisitor& vis) const override;
  void accept(TypePreservingVisitor& vis) override;

  Token source_token;
  bool is_exported;
  std::vector<BoxedTypeAnnot> contents;
};

struct UnionTypeNode : public TypeNode {
  explicit UnionTypeNode(std::vector<BoxedType>&& members) : members(std::move(members)) {
    //
  }
  ~UnionTypeNode() override = default;

  std::string accept(const StringVisitor& vis) const override;
  void accept_const(TypePreservingVisitor& vis) const override;
  void accept(TypePreservingVisitor& vis) override;

  std::vector<BoxedType> members;
};

struct ScalarTypeNode : public TypeNode {
  ScalarTypeNode(const Token& source_token,
                 int64_t identifier,
                 std::vector<BoxedType>&& arguments) :
                 source_token(source_token),
                 identifier(identifier),
                 arguments(std::move(arguments)) {
    //
  }
  ~ScalarTypeNode() override = default;

  std::string accept(const StringVisitor& vis) const override;
  void accept_const(TypePreservingVisitor& vis) const override;
  void accept(TypePreservingVisitor& vis) override;

  Token source_token;
  int64_t identifier;
  std::vector<BoxedType> arguments;
};

struct FunctionTypeNode : public TypeNode {
  FunctionTypeNode(const Token& source_token,
                   std::vector<BoxedType>&& outputs,
                   std::vector<BoxedType>&& inputs) :
                   source_token(source_token),
                   outputs(std::move(outputs)),
                   inputs(std::move(inputs)) {
    //
  }
  ~FunctionTypeNode() override = default;

  std::string accept(const StringVisitor& vis) const override;
  void accept_const(TypePreservingVisitor& vis) const override;
  void accept(TypePreservingVisitor& vis) override;

  Token source_token;
  std::vector<BoxedType> outputs;
  std::vector<BoxedType> inputs;
};

struct FunTypeNode : public TypeNode {
  FunTypeNode(const Token& source_token, BoxedAstNode definition) :
  source_token(source_token), definition(std::move(definition)) {
    //
  }
  ~FunTypeNode() override = default;

  std::string accept(const StringVisitor& vis) const override;
  FunTypeNode* accept(IdentifierClassifier& classifier) override;
  void accept_const(TypePreservingVisitor& vis) const override;
  void accept(TypePreservingVisitor& vis) override;

  Token source_token;
  BoxedAstNode definition;
};

}