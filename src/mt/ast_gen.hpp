#pragma once

#include "ast.hpp"
#include "token.hpp"
#include "Result.hpp"
#include "Optional.hpp"
#include "error.hpp"
#include <vector>
#include <set>
#include <unordered_map>

namespace mt {

class StringRegistry;

/*
 * BlockDepths
 */

struct BlockDepths {
  BlockDepths() :
  function_def(0), for_stmt(0), parfor_stmt(0), if_stmt(0), while_stmt(0), try_stmt(0), switch_stmt(0) {
    //
  }
  ~BlockDepths() = default;

  int function_def;
  int for_stmt;
  int parfor_stmt;
  int if_stmt;
  int while_stmt;
  int try_stmt;
  int switch_stmt;
};

/*
 * MatlabScope
 */

struct MatlabScope {
  explicit MatlabScope(std::shared_ptr<MatlabScope> parent) : parent(std::move(parent)) {
    //
  }
  ~MatlabScope() = default;

  bool register_local_function(int64_t name, FunctionDef* def) {
    if (local_functions.count(name) > 0) {
      return false;
    } else {
      local_functions[name] = def;
      return true;
    }
  }

  void register_local_variable(int64_t name, std::unique_ptr<VariableDef> def) {
    local_variables[name] = std::move(def);
  }

  std::shared_ptr<MatlabScope> parent;
  std::unordered_map<int64_t, FunctionDef*> local_functions;
  std::unordered_map<int64_t, std::unique_ptr<VariableDef>> local_variables;
  std::set<int64_t> imports;
};

/*
 * AstGenerator
 */

class AstGenerator {
  friend struct ParseScopeHelper;
public:
  AstGenerator() : string_registry(nullptr), is_end_terminated_function(true) {
    //
  }

  ~AstGenerator() = default;

  Result<ParseErrors, std::unique_ptr<Block>> parse(const std::vector<Token>& tokens,
                                                    std::string_view text, StringRegistry& registry,
                                                    bool functions_are_end_terminated);
private:
  Optional<std::unique_ptr<Block>> block();
  Optional<std::unique_ptr<Block>> sub_block();
  Optional<std::unique_ptr<FunctionDef>> function_def();
  Optional<FunctionHeader> function_header();
  Optional<std::vector<std::string_view>> function_inputs();
  Optional<std::vector<std::string_view>> function_outputs(bool* provided_outputs);
  Optional<std::string_view> one_identifier();
  Optional<std::vector<std::string_view>> identifier_sequence(TokenType terminator);
  Optional<std::vector<std::string_view>> function_identifier_components();
  Optional<std::vector<Optional<int64_t>>> anonymous_function_input_parameters();

  Optional<Subscript> period_subscript(const Token& source_token);
  Optional<Subscript> non_period_subscript(const Token& source_token, SubscriptMethod method, TokenType term);

  Optional<BoxedExpr> expr(bool allow_empty = false);
  Optional<BoxedExpr> function_expr(const Token& source_token);
  Optional<BoxedExpr> anonymous_function_expr(const Token& source_token);
  Optional<BoxedExpr> function_reference_expr(const Token& source_token);
  Optional<BoxedExpr> grouping_expr(const Token& source_token);
  Optional<BoxedExpr> identifier_reference_expr(const Token& source_token);
  Optional<BoxedExpr> literal_field_reference_expr(const Token& source_token);
  Optional<BoxedExpr> dynamic_field_reference_expr(const Token& source_token);
  Optional<BoxedExpr> ignore_argument_expr(const Token& source_token);
  Optional<BoxedExpr> literal_expr(const Token& source_token);
  Optional<BoxedExpr> colon_subscript_expr(const Token& source_token);
  std::unique_ptr<UnaryOperatorExpr> pending_unary_prefix_expr(const Token& source_token);
  Optional<ParseError> pending_binary_expr(const Token& source_token,
                                           std::vector<BoxedExpr>& completed,
                                           std::vector<BoxedBinaryOperatorExpr>& binaries);
  Optional<ParseError> postfix_unary_expr(const Token& source_token, std::vector<BoxedExpr>& completed);
  Optional<ParseError> handle_postfix_unary_exprs(std::vector<BoxedExpr>& completed);
  void handle_prefix_unary_exprs(std::vector<BoxedExpr>& completed, std::vector<BoxedUnaryOperatorExpr>& unaries);
  void handle_binary_exprs(std::vector<BoxedExpr>& completed,
                           std::vector<BoxedBinaryOperatorExpr>& binaries,
                           std::vector<BoxedBinaryOperatorExpr>& pending_binaries);

  bool is_unary_prefix_expr(const Token& curr_token) const;
  bool is_ignore_argument_expr(const Token& curr_token) const;
  bool is_colon_subscript_expr(const Token& curr_token) const;

  bool is_command_stmt(const Token& curr_token) const;

  Optional<BoxedStmt> assignment_stmt(BoxedExpr lhs, const Token& initial_token);
  Optional<BoxedStmt> expr_stmt(const Token& source_token);
  Optional<BoxedStmt> command_stmt(const Token& source_token);
  Optional<BoxedStmt> if_stmt(const Token& source_token);
  Optional<BoxedStmt> for_stmt(const Token& source_token);
  Optional<BoxedStmt> while_stmt(const Token& source_token);
  Optional<BoxedStmt> switch_stmt(const Token& source_token);
  Optional<BoxedStmt> control_stmt(const Token& source_token);
  Optional<BoxedStmt> try_stmt(const Token& source_token);
  Optional<BoxedStmt> variable_declaration_stmt(const Token& source_token);
  Optional<BoxedStmt> stmt();

  Optional<IfBranch> if_branch(const Token& source_token);
  Optional<SwitchCase> switch_case(const Token& source_token);

  Optional<BoxedTypeAnnot> type_annotation_macro(const Token& source_token);
  Optional<BoxedTypeAnnot> type_annotation(const Token& source_token);
  Optional<BoxedTypeAnnot> type_begin(const Token& source_token);
  Optional<BoxedTypeAnnot> type_given(const Token& source_token);
  Optional<BoxedTypeAnnot> type_let(const Token& source_token);
  Optional<BoxedTypeAnnot> inline_type_annotation(const Token& source_token);

  Optional<BoxedType> type(const Token& source_token);
  Optional<BoxedType> function_type(const Token& source_token);
  Optional<BoxedType> scalar_type(const Token& source_token);
  Optional<std::vector<BoxedType>> type_sequence(TokenType terminator);
  Optional<std::vector<std::string_view>> type_variable_identifiers(const Token& source_token);

  template <size_t N>
  ParseError make_error_expected_token_type(const Token& at_token, const std::array<TokenType, N>& types) const {
    return make_error_expected_token_type(at_token, types.data(), types.size());
  }

  ParseError make_error_expected_token_type(const Token& at_token, const TokenType* types, int64_t num_types) const;
  ParseError make_error_reference_after_parens_reference_expr(const Token& at_token) const;
  ParseError make_error_invalid_expr_token(const Token& at_token) const;
  ParseError make_error_incomplete_expr(const Token& at_token) const;
  ParseError make_error_invalid_assignment_target(const Token& at_token) const;
  ParseError make_error_expected_lhs(const Token& at_token) const;
  ParseError make_error_multiple_exprs_in_parens_grouping_expr(const Token& at_token) const;
  ParseError make_error_duplicate_otherwise_in_switch_stmt(const Token& at_token) const;
  ParseError make_error_expected_non_empty_type_variable_identifiers(const Token& at_token) const;
  ParseError make_error_duplicate_input_parameter_in_expr(const Token& at_token) const;
  ParseError make_error_loop_control_flow_manipulator_outside_loop(const Token& at_token) const;
  ParseError make_error_invalid_function_def_location(const Token& at_token) const;
  ParseError make_error_duplicate_local_function(const Token& at_token) const;

  Optional<ParseError> consume(TokenType type);
  Optional<ParseError> consume_one_of(const TokenType* types, int64_t num_types);
  Optional<ParseError> check_anonymous_function_input_parameters_are_unique(const Token& source_token,
                                                                            const std::vector<Optional<int64_t>>& inputs) const;

  bool is_within_loop() const;
  bool is_within_end_terminated_stmt_block() const;
  bool is_within_function() const;

  void push_scope();
  void pop_scope();
  std::shared_ptr<MatlabScope> current_scope() const;

  void add_error(ParseError&& err);

  static std::array<TokenType, 3> type_annotation_block_possible_types();
  static std::array<TokenType, 5> sub_block_possible_types();

private:
  TokenIterator iterator;
  std::string_view text;
  StringRegistry* string_registry;
  BlockDepths block_depths;
  std::vector<std::shared_ptr<MatlabScope>> scopes;

  bool is_end_terminated_function;

  ParseErrors parse_errors;
};

}