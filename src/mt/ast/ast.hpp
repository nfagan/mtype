#pragma once

#include <memory>

namespace mt {

struct AstNode {
  AstNode() = default;
  virtual ~AstNode() = default;
};

struct Expr : public AstNode {
  Expr() = default;
  ~Expr() override = default;
};

struct Stmt : public AstNode {
  Stmt() = default;
  ~Stmt() override = default;
};

struct Def : public AstNode {
  Def() = default;
  ~Def() override = default;
};

using BoxedAstNode = std::unique_ptr<AstNode>;
using BoxedExpr = std::unique_ptr<Expr>;
using BoxedStmt = std::unique_ptr<Stmt>;
using BoxedDef = std::unique_ptr<Def>;

}