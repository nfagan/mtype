#pragma once

#include <memory>
#include <string>
#include <vector>

namespace mt {

class StringVisitor;

struct AstNode {
  AstNode() = default;
  virtual ~AstNode() = default;

  virtual std::string accept(const StringVisitor& vis) const = 0;
};

template <typename T>
struct is_ast_node : public std::false_type {};
template<>
struct is_ast_node<AstNode> : public std::true_type {};

struct Expr : public AstNode {
  Expr() = default;
  ~Expr() override = default;

  std::string accept(const StringVisitor& vis) const override = 0;
  virtual bool is_valid_assignment_target() const {
    return false;
  }
};

struct Stmt : public AstNode {
  Stmt() = default;
  ~Stmt() override = default;

  std::string accept(const StringVisitor& vis) const override = 0;
};

struct Def : public AstNode {
  Def() = default;
  ~Def() override = default;

  std::string accept(const StringVisitor& vis) const override = 0;
};

using BoxedAstNode = std::unique_ptr<AstNode>;
using BoxedExpr = std::unique_ptr<Expr>;
using BoxedStmt = std::unique_ptr<Stmt>;
using BoxedDef = std::unique_ptr<Def>;

struct Block : public AstNode {
  Block() = default;
  ~Block() override = default;

  void append(BoxedAstNode other) {
    nodes.emplace_back(std::move(other));
  }
  std::string accept(const StringVisitor& vis) const override;

  std::vector<BoxedAstNode> nodes;
};

using BoxedBlock = std::unique_ptr<Block>;

}