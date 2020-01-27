#pragma once

#include <memory>
#include <string>
#include <vector>

namespace mt {

class StringVisitor;
class IdentifierClassifier;

struct AstNode {
  AstNode() = default;
  virtual ~AstNode() = default;

  virtual std::string accept(const StringVisitor& vis) const = 0;
  virtual AstNode* accept(IdentifierClassifier& classifier) = 0;
};

template <typename T>
struct is_ast_node : public std::false_type {};
template<>
struct is_ast_node<AstNode> : public std::true_type {};

struct Expr : public AstNode {
  Expr() = default;
  ~Expr() override = default;

  std::string accept(const StringVisitor& vis) const override = 0;
  Expr* accept(IdentifierClassifier&) override {
    return this;
  }

  virtual bool is_valid_assignment_target() const {
    return false;
  }

  virtual bool append_to_compound_identifier(std::vector<int64_t>&) const {
    return false;
  }
};

struct Stmt : public AstNode {
  Stmt() = default;
  ~Stmt() override = default;

  std::string accept(const StringVisitor& vis) const override = 0;
  Stmt* accept(IdentifierClassifier&) override {
    return this;
  }
};

struct Def : public AstNode {
  Def() = default;
  ~Def() override = default;

  std::string accept(const StringVisitor& vis) const override = 0;
  Def* accept(IdentifierClassifier&) override {
    return this;
  }
};

struct TypeAnnot : public AstNode {
  TypeAnnot() = default;
  ~TypeAnnot() override = default;

  std::string accept(const StringVisitor& vis) const override = 0;
  TypeAnnot* accept(IdentifierClassifier&) override {
    return this;
  }
};

struct Type : public TypeAnnot {
  Type() = default;
  ~Type() override = default;

  std::string accept(const StringVisitor& vis) const override = 0;
};

using BoxedAstNode = std::unique_ptr<AstNode>;
using BoxedExpr = std::unique_ptr<Expr>;
using BoxedStmt = std::unique_ptr<Stmt>;
using BoxedDef = std::unique_ptr<Def>;
using BoxedTypeAnnot = std::unique_ptr<TypeAnnot>;
using BoxedType = std::unique_ptr<Type>;

struct Block : public AstNode {
  Block() = default;
  ~Block() override = default;

  void append(BoxedAstNode other) {
    nodes.emplace_back(std::move(other));
  }
  std::string accept(const StringVisitor& vis) const override;
  Block* accept(IdentifierClassifier& classifier) override;

  std::vector<BoxedAstNode> nodes;
};

using BoxedBlock = std::unique_ptr<Block>;

}