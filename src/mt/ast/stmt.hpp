#pragma once

#include "ast.hpp"

namespace mt {

struct AssignmentStmt : public Stmt {
  AssignmentStmt(BoxedExpr of_expr, BoxedExpr to_expr) :
  of_expr(std::move(of_expr)), to_expr(std::move(to_expr)) {
    //
  }

  ~AssignmentStmt() override = default;
  std::string accept(const StringVisitor& vis) const override;

  BoxedExpr of_expr;
  BoxedExpr to_expr;
};

struct ExprStmt : public Stmt {
  explicit ExprStmt(BoxedExpr expr) : expr(std::move(expr)) {
    //
  }

  ~ExprStmt() override = default;
  std::string accept(const StringVisitor& vis) const override;

  BoxedExpr expr;
};

}