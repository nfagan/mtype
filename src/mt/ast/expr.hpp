#pragma once

#include "ast.hpp"
#include "../lang_components.hpp"
#include "../token.hpp"

namespace mt {

struct DynamicFieldReferenceExpr : public Expr {
  DynamicFieldReferenceExpr(const Token& source_token, BoxedExpr expr) :
  source_token(source_token), expr(std::move(expr)) {
    //
  }
  ~DynamicFieldReferenceExpr() override = default;

  Token source_token;
  BoxedExpr expr;
};

struct LiteralFieldReferenceExpr : public Expr {
  LiteralFieldReferenceExpr(const Token& identifier) : identifier(identifier) {
    //
  }
  ~LiteralFieldReferenceExpr() override = default;

  Token identifier;
};

struct SubscriptExpr : public Expr {
  SubscriptExpr(const Token& source_token, SubscriptMethod method, std::vector<BoxedExpr>&& args) :
  source_token(source_token), method(method), arguments(std::move(args)) {
    //
  }
  ~SubscriptExpr() override = default;

  Token source_token;
  SubscriptMethod method;
  std::vector<BoxedExpr> arguments;
};

struct IdentifierReferenceExpr : public Expr {
  IdentifierReferenceExpr(const Token& identifier, std::vector<std::unique_ptr<SubscriptExpr>>&& subscripts) :
  identifier(identifier), subscripts(std::move(subscripts)) {
    //
  }
  ~IdentifierReferenceExpr() override = default;

  Token identifier;
  std::vector<std::unique_ptr<SubscriptExpr>> subscripts;
};

struct GroupingExprComponent : public Expr {
  GroupingExprComponent(BoxedExpr expr, TokenType delimiter) :
  expr(std::move(expr)), delimiter(delimiter) {
    //
  }

  ~GroupingExprComponent() override = default;

  BoxedExpr expr;
  TokenType delimiter;
};

struct GroupingExpr : public Expr {
  GroupingExpr(const Token& source_token, GroupingMethod method, std::vector<std::unique_ptr<GroupingExprComponent>>&& exprs):
  source_token(source_token), method(method), exprs(std::move(exprs)) {
    //
  }

  Token source_token;
  GroupingMethod method;
  std::vector<std::unique_ptr<GroupingExprComponent>> exprs;
};

struct UnaryOperatorExpr : public Expr {
  UnaryOperatorExpr(const Token& source_token, UnaryOperator op, BoxedExpr expr) :
  source_token(source_token), op(op), expr(std::move(expr)) {
    //
  }
  ~UnaryOperatorExpr() override = default;

  Token source_token;
  UnaryOperator op;
  BoxedExpr expr;
};

struct BinaryOperatorExpr : public Expr {
  BinaryOperatorExpr(const Token& source_token, BinaryOperator op, BoxedExpr left, BoxedExpr right) :
  source_token(source_token), op(op), left(std::move(left)), right(std::move(right)) {

  }
  ~BinaryOperatorExpr() override = default;

  Token source_token;
  BinaryOperator op;
  BoxedExpr left;
  BoxedExpr right;
};

}