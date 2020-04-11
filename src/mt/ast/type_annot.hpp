#pragma once

#include "ast.hpp"
#include "../token.hpp"
#include "../identifier.hpp"

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

  bool represents_type_annot_macro() const override {
    return true;
  }

  Token source_token;
  BoxedTypeAnnot annotation;
};

struct NamespaceTypeNode : public TypeAnnot {
  NamespaceTypeNode(const Token& source_token, const TypeIdentifier& identifier, BoxedTypeAnnots&& enclosing) :
    source_token(source_token), identifier(identifier), enclosing(std::move(enclosing)) {
    //
  }
  ~NamespaceTypeNode() override = default;

  std::string accept(const StringVisitor& vis) const override;
  void accept_const(TypePreservingVisitor& vis) const override;
  void accept(TypePreservingVisitor& vis) override;

  Token source_token;
  TypeIdentifier identifier;
  BoxedTypeAnnots enclosing;
};

struct DeclareTypeNode : public TypeAnnot {
  enum class Kind {
    scalar
  };

  DeclareTypeNode(const Token& source_token, Kind kind, const TypeIdentifier& identifier) :
  source_token(source_token), kind(kind), identifier(identifier) {
    //
  }
  ~DeclareTypeNode() override = default;
  std::string accept(const StringVisitor& vis) const override;
  void accept_const(TypePreservingVisitor& vis) const override;
  void accept(TypePreservingVisitor& vis) override;

  static Optional<Kind> kind_from_string(std::string_view str);

  Token source_token;
  Kind kind;
  TypeIdentifier identifier;
};

struct TypeImportNode : public TypeAnnot {
  TypeImportNode() : import(-1) {
    //
  }

  TypeImportNode(const Token& source_token, int64_t import) :
  source_token(source_token), import(import) {
    //
  }
  ~TypeImportNode() override = default;

  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  int64_t import;
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
  Optional<AstNode*> enclosed_code_ast_node() const override;

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
          const TypeIdentifier& identifier,
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
  TypeIdentifier identifier;
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

struct RecordTypeNode : public TypeNode {
  struct Field {
    Field() = default;
    Field(const TypeIdentifier& name, BoxedType type) : name(name), type(std::move(type)) {
      //
    }

    TypeIdentifier name;
    BoxedType type;
  };

  using Fields = std::vector<Field>;

  RecordTypeNode(const Token& source_token, const Token& name_token,
                 const TypeIdentifier& identifier, Fields&& fields) :
  source_token(source_token), name_token(name_token), identifier(identifier), fields(std::move(fields)) {
    //
  }

  ~RecordTypeNode() override = default;

  std::string accept(const StringVisitor& vis) const override;
  void accept_const(TypePreservingVisitor& vis) const override;
  void accept(TypePreservingVisitor& vis) override;

  Token source_token;
  Token name_token;
  TypeIdentifier identifier;
  Fields fields;
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
                 const TypeIdentifier& identifier,
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
  TypeIdentifier identifier;
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
  Optional<AstNode*> enclosed_code_ast_node() const override;

  Token source_token;
  BoxedAstNode definition;
};

}