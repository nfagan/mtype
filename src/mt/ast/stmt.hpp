#pragma once

#include "ast.hpp"
#include "../token.hpp"

namespace mt {

struct ForStmt : public Stmt {
  ForStmt(const Token& source_token,
          std::string_view loop_variable_identifier,
          BoxedExpr loop_variable_expr,
          BoxedBlock body) :
  source_token(source_token),
  loop_variable_identifier(loop_variable_identifier),
  loop_variable_expr(std::move(loop_variable_expr)),
  body(std::move(body)) {
    //
  }
  ~ForStmt() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  std::string_view loop_variable_identifier;
  BoxedExpr loop_variable_expr;
  BoxedBlock body;
};

struct IfBranch : public AstNode {
  IfBranch(const Token& token, BoxedExpr condition, BoxedBlock block) :
  source_token(token), condition(std::move(condition)), block(std::move(block)) {
    //
  }
  ~IfBranch() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  BoxedExpr condition;
  BoxedBlock block;
};

struct ElseBranch : public AstNode {
  ElseBranch(const Token& source_token, BoxedBlock block) :
  source_token(source_token), block(std::move(block)) {
    //
  }
  ~ElseBranch() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  BoxedBlock block;
};

struct IfStmt : public Stmt {
  IfStmt(std::unique_ptr<IfBranch> if_branch,
         std::vector<std::unique_ptr<IfBranch>>&& elseif_branches,
         std::unique_ptr<ElseBranch> else_branch) :
   if_branch(std::move(if_branch)),
   elseif_branches(std::move(elseif_branches)),
   else_branch(std::move(else_branch)) {
    //
  }
  ~IfStmt() override = default;
  std::string accept(const StringVisitor& vis) const override;

  std::unique_ptr<IfBranch> if_branch;
  std::vector<std::unique_ptr<IfBranch>> elseif_branches;
  std::unique_ptr<ElseBranch> else_branch;
};

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