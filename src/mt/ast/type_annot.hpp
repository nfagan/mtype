#pragma once

#include "ast.hpp"
#include "../token.hpp"
#include "../identifier.hpp"
#include "../lang_components.hpp"
#include <unordered_set>

namespace mt {

class StringRegistry;
struct FunctionTypeNode;
struct AssignmentStmt;
struct IdentifierReferenceExpr;
struct Type;

using BoxedFunctionTypeNode = std::unique_ptr<FunctionTypeNode>;

namespace types {
  struct Record;
  struct Scheme;
}

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

struct ConstructorTypeNode : public TypeAnnot {
  ConstructorTypeNode(const Token& source_token, std::unique_ptr<AssignmentStmt> stmt,
                      IdentifierReferenceExpr* struct_function_call) :
  source_token(source_token), stmt(std::move(stmt)), struct_function_call(struct_function_call) {
    //
  }
  ~ConstructorTypeNode() override = default;
  std::string accept(const StringVisitor& vis) const override;
  ConstructorTypeNode* accept(IdentifierClassifier& classifier) override;
  void accept_const(TypePreservingVisitor& vis) const override;
  void accept(TypePreservingVisitor& vis) override;

  Token source_token;
  std::unique_ptr<AssignmentStmt> stmt;
  IdentifierReferenceExpr* struct_function_call;
};

struct DeclareTypeNode : public TypeAnnot {
  struct Method {
    enum class Kind {
      function,
      unary_operator,
      binary_operator
    };

    Method();
    Method(const TypeIdentifier& name, BoxedFunctionTypeNode type);
    Method(UnaryOperator op, BoxedFunctionTypeNode type);
    Method(BinaryOperator op, BoxedFunctionTypeNode type);

    std::string function_name(const StringRegistry& str_registry) const;

    Kind kind;

    union {
      TypeIdentifier name;
      UnaryOperator unary_operator;
      BinaryOperator binary_operator;
    };

    BoxedFunctionTypeNode type;
  };

  enum class Kind {
    scalar,
    method
  };

  DeclareTypeNode(const Token& source_token, Kind kind, const TypeIdentifier& identifier) :
  source_token(source_token), kind(kind), identifier(identifier) {
    //
  }
  DeclareTypeNode(const Token& source_token, const TypeIdentifier& identifier, Method method) :
  source_token(source_token), kind(Kind::method), identifier(identifier), maybe_method(std::move(method)) {
    //
  }

  ~DeclareTypeNode() override = default;
  std::string accept(const StringVisitor& vis) const override;
  void accept_const(TypePreservingVisitor& vis) const override;
  void accept(TypePreservingVisitor& vis) override;

  static Optional<Kind> kind_from_string(std::string_view str);
  static const char* kind_to_string(Kind kind);

  Token source_token;
  Kind kind;
  TypeIdentifier identifier;
  Method maybe_method;
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
  TypeAssertion(const Token& source_token, BoxedAstNode node, BoxedType has_type, Type* resolved_type) :
  source_token(source_token), node(std::move(node)), has_type(std::move(has_type)), resolved_type(resolved_type) {
    //
  }

  ~TypeAssertion() override = default;

  std::string accept(const StringVisitor& vis) const override;
  TypeAssertion* accept(IdentifierClassifier& classifier) override;
  void accept_const(TypePreservingVisitor& vis) const override;
  void accept(TypePreservingVisitor& vis) override;
  Optional<AstNode*> enclosed_code_ast_node() const override;

  bool is_type_assertion() const override {
    return true;
  }

  Token source_token;
  BoxedAstNode node;
  BoxedType has_type;
  Type* resolved_type;
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
            BoxedTypeAnnot declaration,
            types::Scheme* scheme) :
            source_token(source_token),
            identifiers(std::move(identifiers)),
            declaration(std::move(declaration)),
            scheme(scheme) {
    //
  }
  ~TypeGiven() override = default;

  std::string accept(const StringVisitor& vis) const override;
  void accept_const(TypePreservingVisitor& vis) const override;
  void accept(TypePreservingVisitor& vis) override;

  Token source_token;
  std::vector<int64_t> identifiers;
  BoxedTypeAnnot declaration;
  types::Scheme* scheme;
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
    Field(const Token& source_token, const TypeIdentifier& name, BoxedType type) :
    source_token(source_token), name(name), type(std::move(type)) {
      //
    }

    Token source_token;
    TypeIdentifier name;
    BoxedType type;
  };

  using Fields = std::vector<Field>;
  using FieldNames = std::unordered_set<TypeIdentifier, TypeIdentifier::Hash>;

  RecordTypeNode(const Token& source_token, const Token& name_token,
                 const TypeIdentifier& identifier, types::Record* type, Fields&& fields) :
  source_token(source_token), name_token(name_token), identifier(identifier),
  type(type), fields(std::move(fields)) {
    //
  }

  ~RecordTypeNode() override = default;

  std::string accept(const StringVisitor& vis) const override;
  void accept_const(TypePreservingVisitor& vis) const override;
  void accept(TypePreservingVisitor& vis) override;

  Token source_token;
  Token name_token;
  TypeIdentifier identifier;
  types::Record* type;
  Fields fields;
};

struct InferTypeNode : public TypeNode {
  InferTypeNode(const Token& source_token, Type* type) : source_token(source_token), type(type) {
    //
  }
  ~InferTypeNode() override = default;

  std::string accept(const StringVisitor& vis) const override;
  void accept_const(TypePreservingVisitor& vis) const override;
  void accept(TypePreservingVisitor& vis) override;

  Token source_token;
  Type* type;
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

struct TupleTypeNode : public TypeNode {
  explicit TupleTypeNode(const Token& source_token, BoxedTypes&& members) :
  source_token(source_token), members(std::move(members)) {
    //
  }
  ~TupleTypeNode() override = default;

  std::string accept(const StringVisitor& vis) const override;
  void accept_const(TypePreservingVisitor& vis) const override;
  void accept(TypePreservingVisitor& vis) override;

  Token source_token;
  BoxedTypes members;
};

struct ScalarTypeNode : public TypeNode {
  ScalarTypeNode(const Token& source_token,
                 const TypeIdentifier& identifier,
                 BoxedTypes&& arguments) :
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
  BoxedTypes arguments;
};

struct FunctionTypeNode : public TypeNode {
  FunctionTypeNode(const Token& source_token,
                   std::vector<BoxedType>&& outputs,
                   std::vector<BoxedType>&& inputs,
                   Type* resolved_type) :
                   source_token(source_token),
                   outputs(std::move(outputs)),
                   inputs(std::move(inputs)),
                   resolved_type(resolved_type) {
    //
  }
  ~FunctionTypeNode() override = default;

  std::string accept(const StringVisitor& vis) const override;
  void accept_const(TypePreservingVisitor& vis) const override;
  void accept(TypePreservingVisitor& vis) override;

  bool is_function_type() const override {
    return true;
  }

  Token source_token;
  std::vector<BoxedType> outputs;
  std::vector<BoxedType> inputs;
  Type* resolved_type;
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