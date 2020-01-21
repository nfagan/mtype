#pragma once

#include "ast.hpp"
#include "../lang_components.hpp"
#include "../token.hpp"
#include <algorithm>

namespace mt {

struct AnonymousFunctionExpr : public Expr {
  AnonymousFunctionExpr(const Token& source_token,
                        std::vector<std::string_view>&& input_identifiers,
                        BoxedExpr expr) :
  source_token(source_token),
  input_identifiers(std::move(input_identifiers)),
  expr(std::move(expr)) {
    //
  }
  ~AnonymousFunctionExpr() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  std::vector<std::string_view> input_identifiers;
  BoxedExpr expr;
};

struct FunctionReferenceExpr : public Expr {
  FunctionReferenceExpr(const Token& source_token, BoxedExpr expr) :
  source_token(source_token), expr(std::move(expr)) {
    //
  }
  ~FunctionReferenceExpr() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  BoxedExpr expr;
};

struct ColonSubscriptExpr : public Expr {
  explicit ColonSubscriptExpr(const Token& source_token) : source_token(source_token) {
    //
  }
  ~ColonSubscriptExpr() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
};

struct StringLiteralExpr : public Expr {
  explicit StringLiteralExpr(const Token& source_token) : source_token(source_token) {
    //
  }

  ~StringLiteralExpr() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
};

struct CharLiteralExpr : public Expr {
  explicit CharLiteralExpr(const Token& source_token) : source_token(source_token) {
    //
  }

  ~CharLiteralExpr() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
};

struct NumberLiteralExpr : public Expr {
  NumberLiteralExpr(const Token& source_token, double value) :
  source_token(source_token), value(value) {
    //
  }
  ~NumberLiteralExpr() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  double value;
};

struct IgnoreFunctionArgumentExpr : public Expr {
  explicit IgnoreFunctionArgumentExpr(const Token& source_token) :
  source_token(source_token) {
    //
  }
  ~IgnoreFunctionArgumentExpr() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
};

struct DynamicFieldReferenceExpr : public Expr {
  DynamicFieldReferenceExpr(const Token& source_token, BoxedExpr expr) :
  source_token(source_token), expr(std::move(expr)) {
    //
  }
  ~DynamicFieldReferenceExpr() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  BoxedExpr expr;
};

struct LiteralFieldReferenceExpr : public Expr {
  explicit LiteralFieldReferenceExpr(const Token& identifier) : identifier_token(identifier) {
    //
  }
  ~LiteralFieldReferenceExpr() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token identifier_token;
};

struct SubscriptExpr : public Expr {
  SubscriptExpr(const Token& source_token, SubscriptMethod method, std::vector<BoxedExpr>&& args) :
  source_token(source_token), method(method), arguments(std::move(args)) {
    //
  }
  ~SubscriptExpr() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  SubscriptMethod method;
  std::vector<BoxedExpr> arguments;
};

struct IdentifierReferenceExpr : public Expr {
  IdentifierReferenceExpr(const Token& identifier,
                          std::vector<std::unique_ptr<SubscriptExpr>>&& subscripts) :
  identifier_token(identifier), subscripts(std::move(subscripts)) {
    //
  }
  ~IdentifierReferenceExpr() override = default;

  bool is_valid_assignment_target() const override {
    return true;
  }
  std::string accept(const StringVisitor& vis) const override;

  Token identifier_token;
  std::vector<std::unique_ptr<SubscriptExpr>> subscripts;
};

struct GroupingExprComponent {
  GroupingExprComponent(BoxedExpr expr, TokenType delimiter) :
  expr(std::move(expr)), delimiter(delimiter) {
    //
  }
  GroupingExprComponent(GroupingExprComponent&& other) noexcept = default;
  ~GroupingExprComponent() = default;

  BoxedExpr expr;
  TokenType delimiter;
};

struct GroupingExpr : public Expr {
  GroupingExpr(const Token& source_token, GroupingMethod method,
               std::vector<GroupingExprComponent>&& exprs):
  source_token(source_token), method(method), components(std::move(exprs)) {
    //
  }

  bool is_valid_assignment_target() const override {
    if (method != GroupingMethod::bracket || components.empty()) {
      return false;
    }

    return std::all_of(components.cbegin(), components.cend(), [](const auto& arg) {
      return arg.delimiter == TokenType::comma;
    });
  }
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  GroupingMethod method;
  std::vector<GroupingExprComponent> components;
};

struct EndOperatorExpr : public Expr {
  explicit EndOperatorExpr(const Token& source_token) : source_token(source_token) {
    //
  }
  ~EndOperatorExpr() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
};

struct UnaryOperatorExpr : public Expr {
  UnaryOperatorExpr(const Token& source_token, UnaryOperator op, BoxedExpr expr) :
  source_token(source_token), op(op), expr(std::move(expr)) {
    //
  }
  ~UnaryOperatorExpr() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  UnaryOperator op;
  BoxedExpr expr;
};

struct BinaryOperatorExpr : public Expr {
  BinaryOperatorExpr(const Token& source_token, BinaryOperator op, BoxedExpr left, BoxedExpr right) :
  source_token(source_token), op(op), left(std::move(left)), right(std::move(right)) {

  }
  ~BinaryOperatorExpr() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  BinaryOperator op;
  BoxedExpr left;
  BoxedExpr right;
};

using BoxedUnaryOperatorExpr = std::unique_ptr<UnaryOperatorExpr>;
using BoxedBinaryOperatorExpr = std::unique_ptr<BinaryOperatorExpr>;

}