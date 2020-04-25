#pragma once

#include "ast.hpp"
#include "../lang_components.hpp"
#include "../token.hpp"
#include "../Optional.hpp"
#include "../definitions.hpp"

namespace mt {

class TypePreservingVisitor;

struct FunctionDef;
struct VariableDef;
struct FunctionParameter;
struct IdentifierReferenceExpr;
struct TypeScope;

struct SuperclassMethodReferenceExpr : public Expr {
  SuperclassMethodReferenceExpr(const Token& source_token,
                                const MatlabIdentifier& invoking_argument_name,
                                BoxedExpr superclass_reference_expr) :
                                source_token(source_token),
                                invoking_argument_name(invoking_argument_name),
                                superclass_reference_expr(std::move(superclass_reference_expr)) {
    //
  }
  ~SuperclassMethodReferenceExpr() override = default;

  std::string accept(const StringVisitor& vis) const override;
  Expr* accept(IdentifierClassifier& classifier) override;
  void accept(TypePreservingVisitor& visitor) override;
  void accept_const(TypePreservingVisitor& vis) const override;

  Token source_token;
  MatlabIdentifier invoking_argument_name;
  BoxedExpr superclass_reference_expr;
};

struct AnonymousFunctionExpr : public Expr {
  AnonymousFunctionExpr(const Token& source_token,
                        std::vector<FunctionParameter>&& input_identifiers,
                        BoxedExpr expr,
                        MatlabScope* scope,
                        TypeScope* type_scope) :
    source_token(source_token),
    inputs(std::move(input_identifiers)),
    expr(std::move(expr)),
    scope(scope),
    type_scope(type_scope) {
    //
  }
  ~AnonymousFunctionExpr() override = default;

  std::string accept(const StringVisitor& vis) const override;
  Expr* accept(IdentifierClassifier& classifier) override;
  void accept(TypePreservingVisitor& visitor) override;
  void accept_const(TypePreservingVisitor& vis) const override;

  bool is_anonymous_function_expr() const override {
    return true;
  }

  Token source_token;
  std::vector<FunctionParameter> inputs;
  BoxedExpr expr;
  MatlabScope* scope;
  TypeScope* type_scope;
};

struct FunctionReferenceExpr : public Expr {
  FunctionReferenceExpr(const Token& source_token, const MatlabIdentifier& identifier) :
  source_token(source_token), identifier(identifier) {
    //
  }
  ~FunctionReferenceExpr() override = default;

  std::string accept(const StringVisitor& vis) const override;
  Expr* accept(IdentifierClassifier& classifier) override;
  void accept(TypePreservingVisitor& visitor) override;
  void accept_const(TypePreservingVisitor& vis) const override;

  Token source_token;
  FunctionReferenceHandle handle;
  MatlabIdentifier identifier;
};

struct ColonSubscriptExpr : public Expr {
  explicit ColonSubscriptExpr(const Token& source_token) : source_token(source_token) {
    //
  }
  ~ColonSubscriptExpr() override = default;

  std::string accept(const StringVisitor& vis) const override;
  void accept(TypePreservingVisitor& visitor) override;
  void accept_const(TypePreservingVisitor& vis) const override;

  Token source_token;
};

struct StringLiteralExpr : public Expr {
  explicit StringLiteralExpr(const Token& source_token) : source_token(source_token) {
    //
  }

  ~StringLiteralExpr() override = default;

  std::string accept(const StringVisitor& vis) const override;
  void accept(TypePreservingVisitor& visitor) override;
  void accept_const(TypePreservingVisitor& vis) const override;

  Token source_token;
};

struct CharLiteralExpr : public Expr {
  explicit CharLiteralExpr(const Token& source_token) : source_token(source_token) {
    //
  }

  ~CharLiteralExpr() override = default;

  std::string accept(const StringVisitor& vis) const override;
  void accept(TypePreservingVisitor& visitor) override;
  void accept_const(TypePreservingVisitor& vis) const override;

  bool is_char_literal_expr() const override {
    return true;
  }

  Token source_token;
};

struct NumberLiteralExpr : public Expr {
  NumberLiteralExpr(const Token& source_token, double value) :
  source_token(source_token), value(value) {
    //
  }
  ~NumberLiteralExpr() override = default;

  std::string accept(const StringVisitor& vis) const override;
  void accept(TypePreservingVisitor& visitor) override;
  void accept_const(TypePreservingVisitor& vis) const override;

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
  void accept(TypePreservingVisitor& visitor) override;
  void accept_const(TypePreservingVisitor& vis) const override;

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
  void accept(TypePreservingVisitor& visitor) override;
  void accept_const(TypePreservingVisitor& vis) const override;

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
  void accept(TypePreservingVisitor& visitor) override;
  void accept_const(TypePreservingVisitor& vis) const override;

  bool append_to_compound_identifier(std::vector<int64_t>& prefix) const override {
    prefix.push_back(field_identifier);
    return true;
  }
  bool is_literal_field_reference_expr() const override {
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
                   const FunctionReferenceHandle& reference_handle,
                   std::vector<BoxedExpr>&& arguments,
                   std::vector<Subscript>&& subscripts) :
    source_token(source_token),
    reference_handle(reference_handle),
    arguments(std::move(arguments)),
    subscripts(std::move(subscripts)) {
    //
  }
  ~FunctionCallExpr() override = default;

  std::string accept(const StringVisitor& vis) const override;
  void accept(TypePreservingVisitor& visitor) override;
  void accept_const(TypePreservingVisitor& vis) const override;

  Token source_token;
  FunctionReferenceHandle reference_handle;
  std::vector<BoxedExpr> arguments;
  std::vector<Subscript> subscripts;
};

struct VariableReferenceExpr : public Expr {
  VariableReferenceExpr(const Token& source_token,
                        const VariableDefHandle& def_handle,
                        const MatlabIdentifier& name,
                        std::vector<Subscript>&& subscripts,
                        bool is_initializer) :
  source_token(source_token),
  def_handle(def_handle),
  name(name),
  subscripts(std::move(subscripts)),
  is_initializer(is_initializer) {
    //
  }
  ~VariableReferenceExpr() override = default;

  std::string accept(const StringVisitor& vis) const override;
  void accept(TypePreservingVisitor& visitor) override;
  void accept_const(TypePreservingVisitor& vis) const override;

  Token source_token;
  VariableDefHandle def_handle;
  MatlabIdentifier name;
  std::vector<Subscript> subscripts;
  bool is_initializer;
};

struct IdentifierReferenceExpr : public Expr {
  IdentifierReferenceExpr(const Token& source_token,
                          const MatlabIdentifier& primary_identifier,
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

  bool is_static_identifier_reference_expr() const override;
  bool is_identifier_reference_expr() const override {
    return true;
  }

  std::string accept(const StringVisitor& vis) const override;
  Expr* accept(IdentifierClassifier& classifier) override;
  void accept(TypePreservingVisitor& visitor) override;
  void accept_const(TypePreservingVisitor& vis) const override;

  bool is_non_subscripted_scalar_reference() const;
  bool is_maybe_non_subscripted_function_call() const;
  int64_t num_primary_subscript_arguments() const;

  std::vector<int64_t> make_compound_identifier(int64_t* end) const;

  Optional<const IdentifierReferenceExpr*> extract_identifier_reference_expr() const override;
  Optional<IdentifierReferenceExpr*> extract_mut_identifier_reference_expr() override;

  Token source_token;
  MatlabIdentifier primary_identifier;
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

using GroupingExprComponents = std::vector<GroupingExprComponent>;

struct GroupingExpr : public Expr {
  GroupingExpr(const Token& source_token, GroupingMethod method, GroupingExprComponents&& exprs) :
  source_token(source_token), method(method), components(std::move(exprs)) {
    //
  }

  bool is_valid_assignment_target() const override;
  bool is_grouping_expr() const override {
    return true;
  }

  std::string accept(const StringVisitor& vis) const override;
  Expr* accept(IdentifierClassifier& classifier) override;
  void accept(TypePreservingVisitor& visitor) override;
  void accept_const(TypePreservingVisitor& vis) const override;

  bool is_brace() const;
  bool is_bracket() const;
  bool is_parens() const;

  Token source_token;
  GroupingMethod method;
  GroupingExprComponents components;
};

struct EndOperatorExpr : public Expr {
  explicit EndOperatorExpr(const Token& source_token) : source_token(source_token) {
    //
  }
  ~EndOperatorExpr() override = default;

  std::string accept(const StringVisitor& vis) const override;
  void accept(TypePreservingVisitor& visitor) override;
  void accept_const(TypePreservingVisitor& vis) const override;

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
  void accept(TypePreservingVisitor& visitor) override;
  void accept_const(TypePreservingVisitor& vis) const override;

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
  void accept(TypePreservingVisitor& visitor) override;
  void accept_const(TypePreservingVisitor& vis) const override;

  Token source_token;
  BinaryOperator op;
  BoxedExpr left;
  BoxedExpr right;
};

using BoxedUnaryOperatorExpr = std::unique_ptr<UnaryOperatorExpr>;
using BoxedBinaryOperatorExpr = std::unique_ptr<BinaryOperatorExpr>;

}