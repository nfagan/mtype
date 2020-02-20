#pragma once

#include "ast.hpp"
#include "../token.hpp"
#include "../lang_components.hpp"
#include "../Optional.hpp"
#include "../identifier.hpp"

namespace mt {

struct CharLiteralExpr;

struct VariableDeclarationStmt : public Stmt {
  VariableDeclarationStmt(const Token& source_token,
                          VariableDeclarationQualifier qualifier,
                          std::vector<MatlabIdentifier>&& identifiers) :
                          source_token(source_token),
                          qualifier(qualifier),
                          identifiers(std::move(identifiers)) {
    //
  }
  ~VariableDeclarationStmt() override = default;
  std::string accept(const StringVisitor& vis) const override;
  VariableDeclarationStmt* accept(IdentifierClassifier& classifier) override;

  Token source_token;
  VariableDeclarationQualifier qualifier;
  std::vector<MatlabIdentifier> identifiers;
};

struct CommandStmt : public Stmt {
  //  Constructor defined in source file because CharLiteralExpr is defined in expr.hpp
  CommandStmt(const Token& source_token,
              int64_t identifier,
              std::vector<CharLiteralExpr>&& arguments);
  ~CommandStmt() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  int64_t command_identifier;
  std::vector<CharLiteralExpr> arguments;
};

struct CatchBlock {
  CatchBlock() = default;
  CatchBlock(const Token& source_token, BoxedExpr expr, BoxedBlock block) :
  source_token(source_token), expr(std::move(expr)), block(std::move(block)) {
    //
  }
  CatchBlock(CatchBlock&& other) noexcept = default;
  CatchBlock& operator=(CatchBlock&& other) noexcept = default;
  ~CatchBlock() = default;

  Token source_token;
  BoxedExpr expr;
  BoxedBlock block;
};

struct TryStmt : public Stmt {
  TryStmt(const Token& source_token, BoxedBlock try_block, Optional<CatchBlock>&& catch_block) :
  source_token(source_token),
  try_block(std::move(try_block)),
  catch_block(std::move(catch_block)) {
    //
  }
  ~TryStmt() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  BoxedBlock try_block;
  Optional<CatchBlock> catch_block;
};

struct SwitchCase {
  SwitchCase() = default;
  SwitchCase(const Token& source_token, BoxedExpr expr, BoxedBlock block) :
  source_token(source_token),
  expr(std::move(expr)),
  block(std::move(block)) {
    //
  }
  SwitchCase(SwitchCase&& other) noexcept = default;
  SwitchCase& operator=(SwitchCase&& other) noexcept = default;

  ~SwitchCase() = default;

  Token source_token;
  BoxedExpr expr;
  BoxedBlock block;
};

struct SwitchStmt : public Stmt {
  SwitchStmt(const Token& source_token,
             BoxedExpr condition_expr,
             std::vector<SwitchCase>&& cases,
             BoxedBlock otherwise) :
             source_token(source_token),
             condition_expr(std::move(condition_expr)),
             cases(std::move(cases)),
             otherwise(std::move(otherwise)) {
    //
  }
  ~SwitchStmt() override = default;
  std::string accept(const StringVisitor& vis) const override;
  SwitchStmt* accept(IdentifierClassifier& classifier) override;

  Token source_token;
  BoxedExpr condition_expr;
  std::vector<SwitchCase> cases;
  BoxedBlock otherwise;
};

struct WhileStmt : public Stmt {
  WhileStmt(const Token& source_token, BoxedExpr condition, BoxedBlock body) :
  source_token(source_token),
  condition_expr(std::move(condition)),
  body(std::move(body)) {
    //
  }
  ~WhileStmt() override = default;
  std::string accept(const StringVisitor& vis) const override;
  WhileStmt* accept(IdentifierClassifier& classifier) override;

  Token source_token;
  BoxedExpr condition_expr;
  BoxedBlock body;
};

struct ControlStmt : public Stmt {
  ControlStmt(const Token& source_token, const ControlFlowManipulator kind) :
  source_token(source_token), kind(kind) {
    //
  }
  ~ControlStmt() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  ControlFlowManipulator kind;
};

struct ForStmt : public Stmt {
  ForStmt(const Token& source_token,
          const MatlabIdentifier& loop_variable_identifier,
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
  ForStmt* accept(IdentifierClassifier& classifier) override;

  Token source_token;
  MatlabIdentifier loop_variable_identifier;
  BoxedExpr loop_variable_expr;
  BoxedBlock body;
};

struct IfBranch {
  IfBranch() = default;
  IfBranch(const Token& token, BoxedExpr condition, BoxedBlock block) :
    source_token(token), condition_expr(std::move(condition)), block(std::move(block)) {
    //
  }
  IfBranch(IfBranch&& other) noexcept = default;
  ~IfBranch() = default;

  Token source_token;
  BoxedExpr condition_expr;
  BoxedBlock block;
};

struct ElseBranch {
  ElseBranch() = default;
  ElseBranch(const Token& source_token, BoxedBlock block) :
    source_token(source_token), block(std::move(block)) {
    //
  }
  ElseBranch(ElseBranch&& other) noexcept = default;
  ElseBranch& operator=(ElseBranch&& other) noexcept = default;

  ~ElseBranch() = default;

  Token source_token;
  BoxedBlock block;
};

struct IfStmt : public Stmt {
  IfStmt(IfBranch&& if_branch,
         std::vector<IfBranch>&& elseif_branches,
         Optional<ElseBranch>&& else_branch) :
   if_branch(std::move(if_branch)),
   elseif_branches(std::move(elseif_branches)),
   else_branch(std::move(else_branch)) {
    //
  }
  ~IfStmt() override = default;
  std::string accept(const StringVisitor& vis) const override;
  IfStmt* accept(IdentifierClassifier& classifier) override;

  IfBranch if_branch;
  std::vector<IfBranch> elseif_branches;
  Optional<ElseBranch> else_branch;
};

struct AssignmentStmt : public Stmt {
  AssignmentStmt(BoxedExpr to_expr, BoxedExpr of_expr) :
  of_expr(std::move(of_expr)), to_expr(std::move(to_expr)) {
    //
  }

  ~AssignmentStmt() override = default;
  std::string accept(const StringVisitor& vis) const override;
  AssignmentStmt* accept(IdentifierClassifier& classifier) override;

  BoxedExpr of_expr;
  BoxedExpr to_expr;
};

struct ExprStmt : public Stmt {
  explicit ExprStmt(BoxedExpr expr) : expr(std::move(expr)) {
    //
  }

  ~ExprStmt() override = default;
  std::string accept(const StringVisitor& vis) const override;
  ExprStmt* accept(IdentifierClassifier& classifier) override;

  BoxedExpr expr;
};

}