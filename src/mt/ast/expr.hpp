#pragma once

#include "ast.hpp"
#include "../lang_components.hpp"
#include "../token.hpp"
#include "../Optional.hpp"
#include <algorithm>

namespace mt {

struct FunctionDef;
struct VariableDef;

struct AnonymousFunctionExpr : public Expr {
  AnonymousFunctionExpr(const Token& source_token,
                        std::vector<Optional<int64_t>>&& input_identifiers,
                        BoxedExpr expr,
                        std::shared_ptr<MatlabScope> scope) :
  source_token(source_token),
  input_identifiers(std::move(input_identifiers)),
  expr(std::move(expr)),
  scope(std::move(scope)) {
    //
  }
  ~AnonymousFunctionExpr() override = default;
  std::string accept(const StringVisitor& vis) const override;
  Expr* accept(IdentifierClassifier& classifier) override;

  Token source_token;
  std::vector<Optional<int64_t>> input_identifiers;
  BoxedExpr expr;
  std::shared_ptr<MatlabScope> scope;
};

struct FunctionReferenceExpr : public Expr {
  FunctionReferenceExpr(const Token& source_token, std::vector<int64_t>&& identifier_components) :
  source_token(source_token), identifier_components(std::move(identifier_components)) {
    //
  }
  ~FunctionReferenceExpr() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  std::vector<int64_t> identifier_components;
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
  bool is_valid_assignment_target() const override {
    return true;
  }

  Token source_token;
};

struct DynamicFieldReferenceExpr : public Expr {
  DynamicFieldReferenceExpr(const Token& source_token, BoxedExpr expr) :
  source_token(source_token), expr(std::move(expr)) {
    //
  }
  ~DynamicFieldReferenceExpr() override = default;

  std::string accept(const StringVisitor& vis) const override;
  Expr* accept(IdentifierClassifier& classifier) override;

  Token source_token;
  BoxedExpr expr;
};

struct LiteralFieldReferenceExpr : public Expr {
  explicit LiteralFieldReferenceExpr(const Token& source_token, int64_t field_identifier) :
  source_token(source_token), field_identifier(field_identifier) {
    //
  }
  ~LiteralFieldReferenceExpr() override = default;
  std::string accept(const StringVisitor& vis) const override;
  bool append_to_compound_identifier(std::vector<int64_t>& prefix) const override {
    prefix.push_back(field_identifier);
    return true;
  }

  Token source_token;
  int64_t field_identifier;
};

struct Subscript {
  Subscript() = default;
  Subscript(const Token& source_token, SubscriptMethod method, std::vector<BoxedExpr>&& args) :
  source_token(source_token), method(method), arguments(std::move(args)) {
    //
  }
  Subscript(Subscript&& other) noexcept = default;
  Subscript& operator=(Subscript&& other) noexcept = default;
  ~Subscript() = default;

  Token source_token;
  SubscriptMethod method;
  std::vector<BoxedExpr> arguments;
};

struct FunctionCallExpr : public Expr {
  FunctionCallExpr(const Token& source_token,
                   FunctionReference* function_reference,
                   std::vector<BoxedExpr>&& arguments,
                   std::vector<Subscript>&& subscripts) :
    source_token(source_token),
    function_reference(function_reference),
    arguments(std::move(arguments)),
    subscripts(std::move(subscripts)) {
    //
  }
  ~FunctionCallExpr() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  FunctionReference* function_reference;
  std::vector<BoxedExpr> arguments;
  std::vector<Subscript> subscripts;
};

struct VariableReferenceExpr : public Expr {
  VariableReferenceExpr(const Token& source_token,
                        VariableDef* variable_def,
                        int64_t name,
                        std::vector<Subscript>&& subscripts,
                        bool is_initializer) :
  source_token(source_token),
  variable_def(variable_def),
  name(name),
  subscripts(std::move(subscripts)),
  is_initializer(is_initializer) {
    //
  }
  ~VariableReferenceExpr() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  VariableDef* variable_def;
  int64_t name;
  std::vector<Subscript> subscripts;
  bool is_initializer;
};

struct IdentifierReferenceExpr : public Expr {
  IdentifierReferenceExpr(const Token& source_token,
                          int64_t primary_identifier,
                          std::vector<Subscript>&& subscripts) :
  source_token(source_token),
  primary_identifier(primary_identifier),
  subscripts(std::move(subscripts)) {
    //
  }
  ~IdentifierReferenceExpr() override = default;

  bool is_valid_assignment_target() const override {
    return true;
  }
  std::string accept(const StringVisitor& vis) const override;
  Expr* accept(IdentifierClassifier& classifier) override;

  Token source_token;
  int64_t primary_identifier;
  std::vector<Subscript> subscripts;
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
      return arg.delimiter == TokenType::comma && arg.expr->is_valid_assignment_target();
    });
  }
  std::string accept(const StringVisitor& vis) const override;
  Expr* accept(IdentifierClassifier& classifier) override;

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
  Expr* accept(IdentifierClassifier& classifier) override;

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
  Expr* accept(IdentifierClassifier& classifier) override;

  Token source_token;
  BinaryOperator op;
  BoxedExpr left;
  BoxedExpr right;
};

using BoxedUnaryOperatorExpr = std::unique_ptr<UnaryOperatorExpr>;
using BoxedBinaryOperatorExpr = std::unique_ptr<BinaryOperatorExpr>;

}