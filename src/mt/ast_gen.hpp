#pragma once

#include "ast.hpp"
#include "token.hpp"
#include "Result.hpp"
#include "Optional.hpp"
#include "error.hpp"
#include "store.hpp"
#include "traversal.hpp"
#include <vector>
#include <set>
#include <unordered_map>

namespace mt {

class StringRegistry;
class CodeFileDescriptor;

/*
 * BlockDepths
 */

struct BlockDepths {
  BlockDepths() :
  function_def(0), class_def(0), for_stmt(0), parfor_stmt(0), if_stmt(0), while_stmt(0),
  try_stmt(0), switch_stmt(0), methods(0) {
    //
  }
  ~BlockDepths() = default;

  int function_def;
  int class_def;
  int for_stmt;
  int parfor_stmt;
  int if_stmt;
  int while_stmt;
  int try_stmt;
  int switch_stmt;
  int methods;
};

/*
 * AstGenerator
 */

class AstGenerator {
  friend struct ParseScopeStack;
  friend struct FunctionAttributeStack;
public:
  struct ParseInputs {
    ParseInputs(StringRegistry* string_registry,
                Store* store,
                const CodeFileDescriptor* file_descriptor,
                bool functions_are_end_terminated) :
                string_registry(string_registry),
                store(store),
                file_descriptor(file_descriptor),
                functions_are_end_terminated(functions_are_end_terminated){
      //
    }

    StringRegistry* string_registry;
    Store* store;
    const CodeFileDescriptor* file_descriptor;
    bool functions_are_end_terminated;
  };

public:
  AstGenerator() : AstGenerator(nullptr) {
    //
  }
  explicit AstGenerator(const CodeFileDescriptor* file_descriptor) :
  string_registry(nullptr), store(nullptr), file_descriptor(file_descriptor), is_end_terminated_function(true) {
    //
  }

  ~AstGenerator() = default;

  Result<ParseErrors, BoxedRootBlock> parse(const std::vector<Token>& tokens,
                                            std::string_view text,
                                            const ParseInputs& inputs);
private:
  Optional<std::unique_ptr<Block>> block();
  Optional<std::unique_ptr<Block>> sub_block();
  Optional<std::unique_ptr<FunctionDefNode>> function_def();
  Optional<FunctionHeader> function_header();
  Optional<MatlabIdentifier> compound_function_name(std::string_view first_component);
  Optional<std::vector<FunctionInputParameter>> function_inputs();
  Optional<std::vector<std::string_view>> function_outputs(bool* provided_outputs);
  Optional<std::string_view> one_identifier();
  Optional<std::vector<std::string_view>> identifier_sequence(TokenType terminator);
  Optional<std::vector<std::string_view>> compound_identifier_components();
  Optional<std::vector<FunctionInputParameter>> anonymous_function_input_parameters();

  Optional<BoxedAstNode> class_def();
  Optional<MatlabIdentifier> superclass_name();
  Optional<std::vector<MatlabIdentifier>> superclass_names();

  bool methods_block(std::set<int64_t>& method_names,
                     ClassDef::Methods& methods,
                     std::vector<std::unique_ptr<FunctionDefNode>>& method_def_nodes);
  bool method_def(const Token& source_token,
                  std::set<int64_t>& method_names,
                  ClassDef::Methods& methods,
                  std::vector<std::unique_ptr<FunctionDefNode>>& method_def_nodes);
  bool method_declaration(const Token& source_token,
                          std::set<int64_t>& method_names,
                          ClassDef::Methods& methods);
  bool properties_block(std::set<int64_t>& property_names,
                        std::vector<ClassDef::Property>& properties,
                        std::vector<ClassDefNode::Property>& property_nodes);
  Optional<ClassDefNode::Property> property(const Token& source_token);

  Optional<FunctionAttributes> method_attributes();
  Optional<bool> boolean_attribute_value();
  Optional<AccessSpecifier> access_specifier();

  Optional<MatlabIdentifier> meta_class();

  Optional<Subscript> period_subscript(const Token& source_token);
  Optional<Subscript> non_period_subscript(const Token& source_token, SubscriptMethod method, TokenType term);

  Optional<BoxedExpr> expr(bool allow_empty = false);
  Optional<BoxedExpr> presumed_superclass_method_reference_expr(const Token& source_token,
                                                                BoxedExpr method_reference_expr);
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
  Optional<BoxedStmt> import_stmt(const Token& source_token);
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
  Optional<Import> one_import(const Token& source_token);

  Optional<BoxedTypeAnnot> type_annotation_macro(const Token& source_token);
  Optional<BoxedTypeAnnot> type_annotation(const Token& source_token);
  Optional<BoxedTypeAnnot> type_begin(const Token& source_token);
  Optional<BoxedTypeAnnot> type_given(const Token& source_token);
  Optional<BoxedTypeAnnot> type_let(const Token& source_token);
  Optional<BoxedTypeAnnot> type_fun(const Token& source_token);
  Optional<BoxedTypeAnnot> inline_type_annotation(const Token& source_token);

  Optional<BoxedTypeAnnot> type_fun_enclosing_function(const Token& source_token);
  Optional<BoxedTypeAnnot> type_fun_enclosing_anonymous_function(const Token& source_token);

  Optional<BoxedType> type(const Token& source_token);
  Optional<BoxedType> function_type(const Token& source_token);
  Optional<BoxedType> one_type(const Token& source_token);
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
  ParseError make_error_incomplete_import_stmt(const Token& at_token) const;
  ParseError make_error_invalid_period_qualified_function_def(const Token& at_token) const;
  ParseError make_error_duplicate_class_property(const Token& at_token) const;
  ParseError make_error_duplicate_method(const Token& at_token) const;
  ParseError make_error_duplicate_class_def(const Token& at_token) const;
  ParseError make_error_invalid_superclass_method_reference_expr(const Token& at_token) const;
  ParseError make_error_unrecognized_method_attribute(const Token& at_token) const;
  ParseError make_error_invalid_boolean_attribute_value(const Token& at_token) const;
  ParseError make_error_invalid_access_attribute_value(const Token& at_token) const;
  ParseError make_error_empty_brace_subscript(const Token& at_token) const;
  ParseError make_error_non_assignment_stmt_in_fun_declaration(const Token& at_token) const;
  ParseError make_error_non_anonymous_function_rhs_in_fun_declaration(const Token& at_token) const;
  ParseError make_error_non_identifier_lhs_in_fun_declaration(const Token& at_token) const;

  Optional<ParseError> consume(TokenType type);
  Optional<ParseError> consume_one_of(const TokenType* types, int64_t num_types);
  Optional<ParseError> check_anonymous_function_input_parameters_are_unique(const Token& source_token,
                                                                            const std::vector<FunctionInputParameter>& inputs) const;

  bool is_within_loop() const;
  bool is_within_end_terminated_stmt_block() const;
  bool is_within_function() const;
  bool is_within_top_level_function() const;
  bool is_within_class() const;
  bool is_within_methods() const;
  bool parent_function_is_class_method() const;

  void push_scope();
  void pop_scope();
  MatlabScopeHandle current_scope_handle() const;

  void push_function_attributes(FunctionAttributes&& attrs);
  void pop_function_attributes();
  const FunctionAttributes& current_function_attributes() const;

  void register_import(Import&& import);

  void add_error(ParseError&& err);

  static std::array<TokenType, 4> type_annotation_block_possible_types();
  static std::array<TokenType, 5> sub_block_possible_types();

private:
  TokenIterator iterator;
  std::string_view text;
  StringRegistry* string_registry;
  Store* store;
  const CodeFileDescriptor* file_descriptor;
  BlockDepths block_depths;
  ClassDefState class_state;

  std::vector<MatlabScopeHandle> scope_handles;
  std::vector<FunctionAttributes> function_attributes;

  bool is_end_terminated_function;

  ParseErrors parse_errors;
};

}