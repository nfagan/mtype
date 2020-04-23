#include "ast_gen.hpp"
#include "string.hpp"
#include "keyword.hpp"
#include "type/type_store.hpp"
#include "fs/code_file.hpp"
#include "type/library.hpp"
#include "type/types.hpp"
#include <array>
#include <cassert>
#include <functional>
#include <set>

namespace mt {

namespace {
  inline std::string maybe_prepend_package_name(const ParseInstance* instance, std::string_view ident) {
    if (instance->parent_package.empty()) {
      return std::string(ident);
    } else {
      return instance->parent_package + "." + std::string(ident);
    }
  }

  std::array<TokenType, 5> type_annotation_block_possible_types() {
    return std::array<TokenType, 5>{
      {TokenType::keyword_begin, TokenType::keyword_given, TokenType::keyword_let,
        TokenType::keyword_fun_type, TokenType::double_colon
      }};
  }

  std::array<TokenType, 5> sub_block_possible_types() {
    return {
      {TokenType::keyword_if, TokenType::keyword_for, TokenType::keyword_while,
        TokenType::keyword_try, TokenType::keyword_switch}
    };
  }

  ParseError make_error_expected_token_type(const ParseInstance* instance, const Token& at_token,
                                            const TokenType* types, int64_t num_types) {
    std::vector<std::string> type_strs;

    for (int64_t i = 0; i < num_types; i++) {
      std::string type_str = std::string("`") + to_string(types[i]) + "`";
      type_strs.emplace_back(type_str);
    }

    const auto expected_str = join(type_strs, ", ");
    std::string message = "Expected to receive one of these types: \n\n" + expected_str;
    message += (std::string("\n\nInstead, received: `") + to_string(at_token.type) + "`.");

    return ParseError(instance->source_text(), at_token, std::move(message), instance->file_descriptor());
  }

  template <size_t N>
  ParseError make_error_expected_token_type(const ParseInstance* instance, const Token& at_token,
                                            const std::array<TokenType, N>& types) {
    return make_error_expected_token_type(instance, at_token, types.data(), types.size());
  }

  ParseError make_error_reference_after_parens_reference_expr(const ParseInstance* instance, const Token& at_token) {
    const char* msg = "`()` indexing must appear last in an index expression.";
    return ParseError(instance->source_text(), at_token, msg, instance->file_descriptor());
  }

  ParseError make_error_invalid_expr_token(const ParseInstance* instance, const Token& at_token) {
    const auto type_name = "`" + std::string(to_string(at_token.type)) + "`";
    const auto message = std::string("Token ") + type_name + " is not permitted in expressions.";
    return ParseError(instance->source_text(), at_token, message, instance->file_descriptor());
  }

  ParseError make_error_incomplete_expr(const ParseInstance* instance, const Token& at_token) {
    return ParseError(instance->source_text(), at_token, "Expression is incomplete.", instance->file_descriptor());
  }

  ParseError make_error_invalid_assignment_target(const ParseInstance* instance, const Token& at_token) {
    const auto msg = "The expression on the left is not a valid target for assignment.";
    return ParseError(instance->source_text(), at_token, msg, instance->file_descriptor());
  }

  ParseError make_error_expected_lhs(const ParseInstance* instance, const Token& at_token) {
    const auto msg = "Expected an expression on the left hand side.";
    return ParseError(instance->source_text(), at_token, msg, instance->file_descriptor());
  }

  ParseError make_error_multiple_exprs_in_parens_grouping_expr(const ParseInstance* instance, const Token& at_token) {
    const auto msg = "`()` grouping expressions cannot contain multiple sub-expressions or span multiple lines.";
    return ParseError(instance->source_text(), at_token, msg, instance->file_descriptor());
  }

  ParseError make_error_duplicate_otherwise_in_switch_stmt(const ParseInstance* instance, const Token& at_token) {
    const auto msg = "Duplicate `otherwise` in `switch` statement.";
    return ParseError(instance->source_text(), at_token, msg, instance->file_descriptor());
  }

  ParseError make_error_expected_non_empty_type_variable_identifiers(const ParseInstance* instance, const Token& at_token) {
    const auto msg = "Expected a non-empty list of identifiers.";
    return ParseError(instance->source_text(), at_token, msg, instance->file_descriptor());
  }

  ParseError make_error_duplicate_input_parameter_in_expr(const ParseInstance* instance, const Token& at_token) {
    const auto msg = "Anonymous function contains a duplicate input parameter identifier.";
    return ParseError(instance->source_text(), at_token, msg, instance->file_descriptor());
  }

  ParseError make_error_loop_control_flow_manipulator_outside_loop(const ParseInstance* instance, const Token& at_token) {
    const auto msg = "A `break` or `continue` statement cannot appear outside a loop statement.";
    return ParseError(instance->source_text(), at_token, msg, instance->file_descriptor());
  }

  ParseError make_error_invalid_function_def_location(const ParseInstance* instance, const Token& at_token) {
    const auto msg = "A `function` definition cannot appear here.";
    return ParseError(instance->source_text(), at_token, msg, instance->file_descriptor());
  }

  ParseError make_error_duplicate_local_function(const ParseInstance* instance, const Token& at_token) {
    return ParseError(instance->source_text(), at_token, "Duplicate local function.", instance->file_descriptor());
  }

  ParseError make_error_incomplete_import_stmt(const ParseInstance* instance, const Token& at_token) {
    const auto msg = "An `import` statement must include a compound identifier.";
    return ParseError(instance->source_text(), at_token, msg, instance->file_descriptor());
  }

  ParseError make_error_invalid_period_qualified_function_def(const ParseInstance* instance, const Token& at_token) {
    const auto msg = "A period-qualified `function` is invalid here.";
    return ParseError(instance->source_text(), at_token, msg, instance->file_descriptor());
  }

  ParseError make_error_duplicate_class_property(const ParseInstance* instance, const Token& at_token) {
    return ParseError(instance->source_text(), at_token, "Duplicate property.", instance->file_descriptor());
  }

  ParseError make_error_duplicate_method(const ParseInstance* instance, const Token& at_token) {
    return ParseError(instance->source_text(), at_token, "Duplicate method.", instance->file_descriptor());
  }

  ParseError make_error_duplicate_class_def(const ParseInstance* instance, const Token& at_token) {
    return ParseError(instance->source_text(), at_token, "Duplicate class definition.", instance->file_descriptor());
  }

  ParseError make_error_invalid_superclass_method_reference_expr(const ParseInstance* instance, const Token& at_token) {
    return ParseError(instance->source_text(), at_token, "Invalid superclass method reference.", instance->file_descriptor());
  }

  ParseError make_error_unrecognized_method_attribute(const ParseInstance* instance, const Token& at_token) {
    return ParseError(instance->source_text(), at_token, "Unrecognized method attribute.", instance->file_descriptor());
  }

  ParseError make_error_invalid_boolean_attribute_value(const ParseInstance* instance, const Token& at_token) {
    const auto msg = "Expected attribute value to be one of `true` or `false`.";
    return ParseError(instance->source_text(), at_token, msg, instance->file_descriptor());
  }

  ParseError make_error_invalid_access_attribute_value(const ParseInstance* instance, const Token& at_token) {
    return ParseError(instance->source_text(), at_token, "Unrecognized access attribute value.", instance->file_descriptor());
  }

  ParseError make_error_empty_brace_subscript(const ParseInstance* instance, const Token& at_token) {
    return ParseError(instance->source_text(), at_token, "`{}` subscripts require arguments.", instance->file_descriptor());
  }

  ParseError make_error_non_assignment_stmt_in_fun_declaration(const ParseInstance* instance, const Token& at_token) {
    const auto msg = "`fun` definition must precede an assignment statement.";
    return ParseError(instance->source_text(), at_token, msg, instance->file_descriptor());
  }

  ParseError make_error_non_anonymous_function_rhs_in_fun_declaration(const ParseInstance* instance, const Token& at_token) {
    const auto msg = "`fun` definition rhs must be an anonymous function.";
    return ParseError(instance->source_text(), at_token, msg, instance->file_descriptor());
  }

  ParseError make_error_non_identifier_lhs_in_fun_declaration(const ParseInstance* instance, const Token& at_token) {
    const auto msg = "`fun` definition lhs must be a scalar identifier.";
    return ParseError(instance->source_text(), at_token, msg, instance->file_descriptor());
  }

  ParseError make_error_non_scalar_outputs_in_ctor(const ParseInstance* instance, const Token& at_token) {
    const auto msg = "A class constructor must have exactly one output parameter.";
    return ParseError(instance->source_text(), at_token, msg, instance->file_descriptor());
  }

  ParseError make_error_no_class_instance_parameter_in_method(const ParseInstance* instance, const Token& at_token) {
    const auto msg = "A non-static method must have at least one input parameter.";
    return ParseError(instance->source_text(), at_token, msg, instance->file_descriptor());
  }

  ParseError make_error_duplicate_type_identifier(const ParseInstance* instance, const Token& at_token) {
    std::string msg = make_error_message_duplicate_type_identifier(at_token.lexeme);
    return ParseError(instance->source_text(), at_token, msg, instance->file_descriptor());
  }

  ParseError make_error_duplicate_record_field_name(const ParseInstance* instance, const Token& at_token) {
    std::string msg = "Duplicate field name `" + std::string(at_token.lexeme) + "`.";
    return ParseError(instance->source_text(), at_token, msg, instance->file_descriptor());
  }

  ParseError make_error_unrecognized_type_declaration_kind(const ParseInstance* instance, const Token& at_token) {
    return ParseError(instance->source_text(), at_token, "Unrecognized declaration kind.", instance->file_descriptor());
  }

  ParseError make_error_expected_function_type(const ParseInstance* instance, const Token& at_token) {
    return ParseError(instance->source_text(), at_token, "Expected function type.", instance->file_descriptor());
  }

  ParseError make_error_expected_type_assertion(const ParseInstance* instance, const Token& at_token) {
    return ParseError(instance->source_text(), at_token, "Expected type assertion.", instance->file_descriptor());
  }

  ParseError make_error_expected_assignment_stmt_in_ctor(const ParseInstance* instance, const Token& at_token) {
    const auto msg = "Expected assignment statement following constructor statement.";
    return ParseError(instance->source_text(), at_token, msg, instance->file_descriptor());
  }

  ParseError make_error_expected_identifier_reference_expr_in_ctor(const ParseInstance* instance, const Token& at_token) {
    const auto msg = "Expected identifier reference expression following constructor statement.";
    return ParseError(instance->source_text(), at_token, msg, instance->file_descriptor());
  }

  ParseError make_error_expected_struct_function_call(const ParseInstance* instance, const Token& at_token) {
    const auto msg = "A constructor statement must include a call to `struct`, without subscripts.";
    return ParseError(instance->source_text(), at_token, msg, instance->file_descriptor());
  }

  ParseError make_error_expected_even_number_of_arguments_in_ctor(const ParseInstance* instance, const Token& at_token) {
    const auto msg = "Expected an even number of arguments in `struct` call.";
    return ParseError(instance->source_text(), at_token, msg, instance->file_descriptor());
  }

  ParseError make_error_stmts_preceding_class_def(const ParseInstance* instance, const Token& at_token) {
    const auto msg = "A classdef file cannot have functions or statements preceding the definition.";
    return ParseError(instance->source_text(), at_token, msg, instance->file_descriptor());
  }

  ParseError make_error_invalid_typed_property(const ParseInstance* instance, const Token& at_token) {
    const auto msg = "Invalid typed property.";
    return ParseError(instance->source_text(), at_token, msg, instance->file_descriptor());
  }

  ParseError make_error_varargout_in_incorrect_position(const ParseInstance* instance, const Token& token) {
    const auto msg = "`varargout` must be the last output parameter.";
    return ParseError(instance->source_text(), token, msg, instance->file_descriptor());
  }

  ParseError make_error_varargin_in_incorrect_position(const ParseInstance* instance, const Token& token) {
    const auto msg = "`varargin` must be the last input parameter.";
    return ParseError(instance->source_text(), token, msg, instance->file_descriptor());
  }
}

/*
 * PendingExternalMethod
 */

PendingExternalMethod::PendingExternalMethod(ClassDefNode::MethodDeclaration method_decl,
                                             const MatlabIdentifier& method_name,
                                             const Token& name_token,
                                             const FunctionAttributes& method_attributes,
                                             const ClassDefHandle& class_def_handle,
                                             const MatlabIdentifier& class_name,
                                             ClassDefNode* class_node,
                                             MatlabScope* matlab_scope,
                                             TypeScope* type_scope) :
    method_declaration(std::move(method_decl)),
    method_name(method_name),
    name_token(name_token),
    method_attributes(method_attributes),
    class_def_handle(class_def_handle),
    class_name(class_name),
    class_node(class_node),
    matlab_scope(matlab_scope),
    type_scope(type_scope) {
    //
  }

/*
 * ParseInstance
 */

ParseInstance::ParseInstance(Store* store,
                             TypeStore* type_store,
                             Library* library,
                             StringRegistry* string_registry,
                             const ParseSourceData& source_data,
                             bool functions_are_end_terminated,
                             OnBeforeParse on_before_parse) :
  store(store),
  type_store(type_store),
  library(library),
  string_registry(string_registry),
  source_data(source_data),
  functions_are_end_terminated(functions_are_end_terminated),
  treat_root_as_external_method(false),
  on_before_parse(std::move(on_before_parse)),
  had_error(false),
  num_visited_functions(0),
  registered_file_type(false),
  file_type(CodeFileType::unknown),
  file_entry_function_def_node(nullptr) {
  parent_package = source_data.file_descriptor->containing_package();
}

bool ParseInstance::has_parent_package() const {
  return !parent_package.empty();
}

void ParseInstance::mark_visited_function() {
  num_visited_functions++;
}

bool ParseInstance::register_file_type(CodeFileType ft) {
  if (registered_file_type) {
    return false;
  }

  file_type = ft;
  registered_file_type = true;
  return true;
}

void ParseInstance::set_file_entry_function_ref(const FunctionReferenceHandle& ref_handle,
                                                FunctionDefNode* file_entry_node) {
  assert(!file_entry_function_ref.is_valid() && !file_entry_function_def_node);
  file_entry_function_ref = ref_handle;
  file_entry_function_def_node = file_entry_node;
}
void ParseInstance::set_file_entry_class_def(const ClassDefHandle& def_handle) {
  assert(!file_entry_class_def.is_valid());
  file_entry_class_def = def_handle;
}

bool ParseInstance::is_function_file() const {
  return file_type == CodeFileType::function_def;
}

bool ParseInstance::is_class_file() const {
  return file_type == CodeFileType::class_def;
}

bool ParseInstance::is_file_entry_function() const {
  return is_function_file() && num_visited_functions == 1;
}

const CodeFileDescriptor* ParseInstance::file_descriptor() const {
  return source_data.file_descriptor;
}

std::string_view ParseInstance::source_text() const {
  return source_data.source;
}

struct BlockStmtScopeHelper {
  BlockStmtScopeHelper(int* scope) : scope(scope) {
    (*scope)++;
  }
  ~BlockStmtScopeHelper() {
    (*scope)--;
  };

  int* scope;
};

struct ParseScopeStack {
  static void push(AstGenerator& gen) {
    gen.push_scope();
  }
  static void pop(AstGenerator& gen) {
    gen.pop_scope();
  }
};

struct ParseScopeHelper : public StackHelper<AstGenerator, ParseScopeStack> {
  using StackHelper::StackHelper;
};

struct FunctionAttributeStack {
  static void push(AstGenerator&) {
    //
  }
  static void pop(AstGenerator& gen) {
    gen.pop_function_attributes();
  }
};

struct FunctionAttributeHelper : public StackHelper<AstGenerator, FunctionAttributeStack> {
  using StackHelper::StackHelper;
};

/*
 * AstGenerator
 */

AstGenerator::AstGenerator(ParseInstance* instance, const std::vector<Token>& tokens) :
  parse_instance(instance),
  iterator(TokenIterator(&tokens)),
  string_registry(instance->string_registry),
  store(instance->store) {
  push_default_state();
}

void AstGenerator::push_default_state() {
  //  No enclosing scheme
  enclosing_schemes.push_back(nullptr);

  //  Push root scope.
  push_scope();

  //  Push default function attributes.
  push_function_attributes(FunctionAttributes());

  //  Push non-exported type identifiers.
  type_identifier_export_state.push_non_export();
}

void AstGenerator::parse() {
  auto result = block();

  if (result) {
    auto block_node = std::move(result.rvalue());
    auto root_node = std::make_unique<RootBlock>(std::move(block_node), current_scope(), current_type_scope());
    parse_instance->root_block = std::move(root_node);
  } else {
    parse_instance->had_error = true;
  }
}

Optional<std::string_view> AstGenerator::one_identifier() {
  auto tok = iterator.peek();
  auto err = consume(TokenType::identifier);

  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  } else {
    return Optional<std::string_view>(tok.lexeme);
  }
}

Optional<std::vector<FunctionParameter>> AstGenerator::function_parameters(bool* has_varargin) {
  std::vector<FunctionParameter> parameters;
  bool expect_parameter = true;
  *has_varargin = false;

  int64_t iter = 0;
  int64_t varargin_index = -1;
  const auto varargin_ident = parse_instance->library->special_identifiers.varargin;
  Token varargin_token;

  while (iterator.has_next() && iterator.peek().type != TokenType::right_parens) {
    const auto& tok = iterator.peek();
    Optional<ParseError> err;

    if (expect_parameter) {
      std::array<TokenType, 2> possible_types{{TokenType::identifier, TokenType::tilde}};
      err = consume_one_of(possible_types.data(), possible_types.size());
    } else {
      err = consume(TokenType::comma);
    }

    if (err) {
      add_error(err.rvalue());
      return NullOpt{};
    }

    if (tok.type == TokenType::identifier) {
      const auto full_name = string_registry->register_string(tok.lexeme);
      auto param = FunctionParameter(MatlabIdentifier(full_name));

      if (full_name == varargin_ident) {
        varargin_index = iter;
        varargin_token = tok;
        *has_varargin = true;
        param.mark_is_vararg();
      }

      parameters.push_back(param);

    } else if (tok.type == TokenType::tilde) {
      //  @(~, y)
      parameters.emplace_back();
    }

    expect_parameter = !expect_parameter;
    iter++;
  }

  auto err = consume(TokenType::right_parens);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  //  If function has a `varargin` parameter, it must be the last one.
  if (varargin_index >= 0 && (varargin_index + 1) != iter) {
    add_error(make_error_varargin_in_incorrect_position(parse_instance, varargin_token));
    return NullOpt{};
  }

  return Optional<std::vector<FunctionParameter>>(std::move(parameters));
}

Optional<std::vector<std::string_view>> AstGenerator::compound_identifier_components() {
  //  @Note: We can't just use the identical identifier sequence logic from below, because
  //  there's no one expected terminator here.
  std::vector<std::string_view> identifiers;
  bool expect_identifier = true;

  while (iterator.has_next()) {
    const auto& tok = iterator.peek();

    const auto expected_type = expect_identifier ? TokenType::identifier : TokenType::period;
    if (tok.type != expected_type) {
      if (expect_identifier) {
        add_error(make_error_expected_token_type(parse_instance, tok, &expected_type, 1));
        return NullOpt{};
      } else {
        break;
      }
    }

    iterator.advance();

    if (tok.type == TokenType::identifier) {
      identifiers.emplace_back(tok.lexeme);
    }

    expect_identifier = !expect_identifier;
  }

  return Optional<std::vector<std::string_view>>(std::move(identifiers));
}

Optional<std::vector<std::string_view>> AstGenerator::identifier_sequence(TokenType terminator) {
  std::vector<std::string_view> identifiers;
  bool expect_identifier = true;

  while (iterator.has_next() && iterator.peek().type != terminator) {
    const auto& tok = iterator.peek();

    const auto expected_type = expect_identifier ? TokenType::identifier : TokenType::comma;
    auto err = consume(expected_type);
    if (err) {
      add_error(err.rvalue());
      return NullOpt{};
    }

    if (expect_identifier) {
      identifiers.push_back(tok.lexeme);
    }

    expect_identifier = !expect_identifier;
  }

  auto term_err = consume(terminator);
  if (term_err) {
    add_error(term_err.rvalue());
    return NullOpt{};
  } else {
    return Optional<std::vector<std::string_view>>(std::move(identifiers));
  }
}

Optional<MatlabIdentifier> AstGenerator::compound_function_name(std::string_view first_component) {
  iterator.advance();

  auto property_res = compound_identifier_components();
  if (!property_res) {
    return NullOpt{};
  }

  auto component_names = std::move(property_res.rvalue());
  component_names.insert(component_names.begin(), first_component);
  const auto component_ids = string_registry->register_strings(component_names);
  const auto compound_name = string_registry->make_registered_compound_identifier(component_ids);

  return Optional<MatlabIdentifier>(MatlabIdentifier(compound_name, component_ids.size()));
}

Optional<FunctionHeader> AstGenerator::function_header(bool* has_varargin, bool* has_varargout) {
  //  @TODO: Ensure parameter names are unique.
  bool provided_outputs;

  auto output_res = function_outputs(&provided_outputs, has_varargout);
  if (!output_res) {
    return NullOpt{};
  }

  if (provided_outputs) {
    auto err = consume(TokenType::equal);
    if (err) {
      add_error(err.rvalue());
      return NullOpt{};
    }
  }

  const auto& name_token = iterator.peek();
  auto name_res = one_identifier();
  if (!name_res) {
    return NullOpt{};
  }

  MatlabIdentifier name_id(string_registry->register_string(name_res.value()));
  MatlabIdentifier name;
  bool use_compound_name = false;

  if (iterator.peek().type == TokenType::period) {
    //  e.g., set.<property>, when within a methods block
    if (is_within_class() && is_within_top_level_function()) {
      auto compound_res = compound_function_name(name_res.value());
      if (!compound_res) {
        return NullOpt{};
      }

      name = compound_res.value();
      use_compound_name = true;

    } else {
      add_error(make_error_invalid_period_qualified_function_def(parse_instance, iterator.peek()));
      return NullOpt{};
    }
  }

  auto input_res = function_inputs(has_varargin);
  if (!input_res) {
    return NullOpt{};
  }

  if (parse_instance->has_parent_package() && is_within_top_level_function() && is_within_class()) {
    //  If the class is defined within a package, check to see whether the method's name matches
    //  the non-package-qualified class name. If so, this method is the constructor, and it is
    //  registered using the package prefix. Other class methods are not package-prefixed.
    auto unqualified_class_name = class_state.unqualified_enclosing_class_name();
    auto maybe_package_prefixed_name = maybe_prepend_package_name(parse_instance, name_res.value());

    if (maybe_package_prefixed_name != name_res.value() &&
        name_id == unqualified_class_name) {
      name = MatlabIdentifier(string_registry->register_string(maybe_package_prefixed_name));
      use_compound_name = true;
    }

  } else if (parse_instance->has_parent_package() && parse_instance->is_file_entry_function()) {
    auto maybe_package_prefixed_name = maybe_prepend_package_name(parse_instance, name_res.value());
    name = MatlabIdentifier(string_registry->register_string(maybe_package_prefixed_name));
    use_compound_name = true;
  }

  if (!use_compound_name) {
    auto single_name_component = string_registry->register_string(name_res.value());
    name = MatlabIdentifier(single_name_component);
  }

  FunctionHeader header(name_token, name, std::move(output_res.rvalue()), std::move(input_res.rvalue()));
  return Optional<FunctionHeader>(std::move(header));
}

Optional<FunctionParameters> AstGenerator::function_inputs(bool* has_varargin) {
  if (iterator.peek().type == TokenType::left_parens) {
    iterator.advance();
    return function_parameters(has_varargin);
  } else {
    *has_varargin = false;
    //  Function declarations without parentheses are valid and indicate 0 inputs.
    return Optional<FunctionParameters>(FunctionParameters{});
  }
}

Optional<FunctionParameters> AstGenerator::function_outputs(bool* provided_outputs, bool* has_varargout) {
  const auto& tok = iterator.peek();
  std::vector<std::string_view> parameter_strs;
  *provided_outputs = false;
  *has_varargout = false;

  if (tok.type == TokenType::left_bracket) {
    //  function [a, b, c] = example()
    iterator.advance();
    *provided_outputs = true;
    auto seq_res = identifier_sequence(TokenType::right_bracket);
    if (!seq_res) {
      return NullOpt{};
    } else {
      parameter_strs = std::move(seq_res.rvalue());
    }

  } else if (tok.type == TokenType::identifier) {
    //  Either: function a = example() or function example(). I.e., the next identifier is either
    //  the single output parameter of the function or the name of the function.
    const auto& next_tok = iterator.peek_nth(1);
    const bool is_single_output = next_tok.type == TokenType::equal;

    if (is_single_output) {
      *provided_outputs = true;
      parameter_strs.push_back(tok.lexeme);
      iterator.advance();
    }

  } else {
    std::array<TokenType, 2> possible_types{{TokenType::left_bracket, TokenType::identifier}};
    add_error(make_error_expected_token_type(parse_instance, tok, possible_types));
    return NullOpt{};
  }

  FunctionParameters outputs;
  outputs.reserve(parameter_strs.size());
  const auto varargout_ident = parse_instance->library->special_identifiers.varargout;
  const int64_t num_params = parameter_strs.size();

  for (int64_t i = 0; i < num_params; i++) {
    const auto full_name = string_registry->register_string(parameter_strs[i]);
    auto param = FunctionParameter(MatlabIdentifier(full_name));

    //  `varargout` must be last output parameter.
    if (full_name == varargout_ident) {
      if (i != num_params-1) {
        add_error(make_error_varargout_in_incorrect_position(parse_instance, tok));
        return NullOpt{};
      } else {
        *has_varargout = true;
        param.mark_is_vararg();
      }
    }

    outputs.push_back(param);
  }

  return Optional<FunctionParameters>(std::move(outputs));
}

Optional<std::unique_ptr<FunctionDefNode>> AstGenerator::function_def() {
  const auto& source_token = iterator.peek();
  iterator.advance();

  if (!parse_instance->registered_file_type) {
    parse_instance->register_file_type(CodeFileType::function_def);
  }
  parse_instance->mark_visited_function();

  //  Enter function keyword
  BlockStmtScopeHelper block_depth_helper(&block_depths.function_def);

  if (is_within_end_terminated_stmt_block()) {
    //  It's illegal to define a function within an if statement, etc.
    add_error(make_error_invalid_function_def_location(parse_instance, source_token));
    return NullOpt{};
  }

  bool has_varargin;
  bool has_varargout;
  auto header_result = function_header(&has_varargin, &has_varargout);

  if (!header_result) {
    return NullOpt{};
  }

  Optional<std::unique_ptr<Block>> body_res;
  MatlabScope* child_scope = nullptr;
  TypeScope* child_type_scope = nullptr;

  {
    //  Increment scope, and decrement upon block exit.
    ParseScopeHelper scope_helper(*this);
    child_scope = current_scope();
    child_type_scope = current_type_scope();

    body_res = sub_block();
    if (!body_res) {
      return NullOpt{};
    }

    if (parse_instance->functions_are_end_terminated) {
      auto err = consume(TokenType::keyword_end);
      if (err) {
        add_error(err.rvalue());
        return NullOpt{};
      }
    }
  }

  auto attrs = current_function_attributes();
  const auto& header = header_result.value();
  const auto name = header.name;

  if (is_within_class() && is_within_top_level_function()) {
    //  Check whether this method is the constructor.
    auto curr_class_name = class_state.enclosing_class_name();
    const bool is_ctor = curr_class_name == name;

    if (is_ctor) {
      attrs.mark_constructor();

      if (header.outputs.size() != 1) {
        //  Constructor of the form [] = (arg...) is invalid.
        add_error(make_error_non_scalar_outputs_in_ctor(parse_instance, header.name_token));
        return NullOpt{};
      }
    } else if (!attrs.is_static() && header.inputs.empty()) {
      //  Non-static method must accept at least one argument (the class instance).
      add_error(make_error_no_class_instance_parameter_in_method(parse_instance, header.name_token));
      return NullOpt{};
    }
  }

  attrs.maybe_mark_has_varargin(has_varargin);
  attrs.maybe_mark_has_varargout(has_varargout);

  FunctionDef function_def(header_result.rvalue(), attrs, body_res.rvalue());

  //  Function registry owns the local function definition.
  FunctionDefHandle def_handle;
  FunctionReferenceHandle ref_handle;

  {
    Store::Write writer(*store);
    def_handle = writer.emplace_definition(std::move(function_def));
    ref_handle = writer.make_local_reference(name, def_handle, child_scope);
  }

  Store::ReadMut reader(*store);
  auto& parent_scope = *current_scope();

  bool register_success = true;

  if (root_is_external_method() || !is_within_class() || !is_within_top_level_function()) {
    //  In a classdef file, adjacent top-level methods are not visible to one another, so we don't
    //  register them in the parent's scope.
    register_success = parent_scope.register_local_function(name, ref_handle);
  }

  if (!register_success) {
    add_error(make_error_duplicate_local_function(parse_instance, source_token));
    return NullOpt{};
  }

  auto ast_node = std::make_unique<FunctionDefNode>(source_token, def_handle, ref_handle,
                                                    child_scope, child_type_scope);

  //  Mark this function as the function accessible to external files by its filename.
  if (parse_instance->is_file_entry_function() || attrs.is_constructor()) {
    parse_instance->set_file_entry_function_ref(ref_handle, ast_node.get());
  }

  return Optional<std::unique_ptr<FunctionDefNode>>(std::move(ast_node));
}

Optional<MatlabIdentifier> AstGenerator::superclass_name() {
  auto superclass_res = compound_identifier_components();
  if (!superclass_res) {
    return NullOpt{};
  }

  const auto& names = superclass_res.value();
  std::vector<int64_t> name_ids;
  name_ids.reserve(names.size());

  for (const auto& name : names) {
    name_ids.emplace_back(string_registry->register_string(name));
  }

  const auto full_name = string_registry->make_registered_compound_identifier(name_ids);
  MatlabIdentifier identifier(full_name, name_ids.size());

  return Optional<MatlabIdentifier>(identifier);
}

Optional<ClassDef::Superclasses> AstGenerator::superclasses() {
  ClassDef::Superclasses supers;

  while (iterator.has_next()) {
    auto one_name_res = superclass_name();
    if (!one_name_res) {
      return NullOpt{};
    }

    if (!parse_instance->library->is_builtin_class(one_name_res.value())) {
      ClassDef::Superclass super{one_name_res.value(), ClassDefHandle()};
      supers.push_back(super);
    }

    if (iterator.peek().type == TokenType::ampersand) {
      iterator.advance();
    } else {
      break;
    }
  }

  return Optional<ClassDef::Superclasses>(std::move(supers));
}

namespace {
  void add_pending_external_methods(ParseInstance* parse_instance,
                                    ClassDefNode::MethodDeclarations& method_decls,
                                    const ClassDefHandle& class_handle,
                                    const MatlabIdentifier& class_name,
                                    ClassDefNode* class_node,
                                    MatlabScope* matlab_scope,
                                    TypeScope* type_scope) {
    for (auto& decl : method_decls) {
      MatlabIdentifier name;
      FunctionAttributes method_attribs;
      Token name_token;

      parse_instance->store->use<Store::ReadConst>([&](const auto& reader) {
        const auto& def = reader.at(decl.pending_def_handle);
        name = def.header.name;
        method_attribs = def.attributes;
        name_token = def.header.name_token;
      });

      PendingExternalMethod pending_method(std::move(decl), name, name_token, method_attribs,
                                           class_handle, class_name, class_node,
                                           matlab_scope, type_scope);

      parse_instance->pending_external_methods.push_back(std::move(pending_method));
    }
  }
}

void AstGenerator::enter_methods_block_via_external_method(const PendingExternalMethod& method) {
  scopes.clear();
  type_scopes.clear();

  assert(method.matlab_scope && !method.matlab_scope->parent);
  assert(method.type_scope && !method.type_scope->parent && method.type_scope->is_root());

  scopes.push_back(method.matlab_scope);
  type_scopes.push_back(method.type_scope);

  assert(root_type_scope() == method.type_scope);
  assert(root_scope() == method.matlab_scope);

  block_depths.class_def++;
  block_depths.class_def_file++;
  block_depths.methods++;

  push_function_attributes(method.method_attributes);
  class_state.push_class(method.class_def_handle, method.class_name);
}

Optional<BoxedAstNode> AstGenerator::class_def() {
  const auto& source_token = iterator.peek();
  iterator.advance();

  if (!parse_instance->register_file_type(CodeFileType::class_def)) {
    add_error(make_error_stmts_preceding_class_def(parse_instance, source_token));
    return NullOpt{};
  }

  const auto parent_scope = current_scope();
  auto parent_type_scope = current_type_scope();
  BlockStmtScopeHelper scope_helper(&block_depths.class_def);
  block_depths.class_def_file++;

  if (iterator.peek().type == TokenType::left_parens) {
    //  @TODO: Handle class attributes.
    //  e.g., classdef (Sealed)
    const auto r_parens = TokenType::right_parens;
    iterator.advance_to_one(&r_parens, 1);
    auto err = consume(r_parens);
    if (err) {
      add_error(err.rvalue());
      return NullOpt{};
    }
  }

  const auto name_token = iterator.peek();
  auto name_res = one_identifier();
  if (!name_res) {
    return NullOpt{};
  }

  ClassDef::Superclasses supers;

  //  classdef X < Y
  if (iterator.peek().type == TokenType::less) {
    iterator.advance();

    auto superclass_res = superclasses();
    if (!superclass_res) {
      return NullOpt{};
    }

    supers = std::move(superclass_res.rvalue());
  }

  ClassDef::Properties properties;
  ClassDef::Methods methods;
  ClassDefNode::MethodDeclarations method_decls;

  BoxedMethodNodes method_def_nodes;
  BoxedPropertyNodes property_nodes;

  std::set<int64_t> property_names;
  std::set<int64_t> method_names;

  ClassDefHandle class_handle;

  {
    Store::Write writer(*store);
    class_handle = writer.make_class_definition();
  }

  const auto& given_name = name_res.value();
  const auto package_qualified_name = maybe_prepend_package_name(parse_instance, given_name);

  MatlabIdentifier unqualified_name(string_registry->register_string(given_name));
  MatlabIdentifier qualified_name(string_registry->register_string(package_qualified_name));

  ClassDefState::Helper class_helper(class_state, class_handle, nullptr, qualified_name, unqualified_name);

  while (iterator.has_next() && iterator.peek().type != TokenType::keyword_end) {
    const auto& tok = iterator.peek();

    if (can_be_skipped_in_classdef_block(tok.type)) {
      iterator.advance();

    } else if (tok.type == TokenType::keyword_properties) {
      auto prop_res = properties_block(property_names, properties, property_nodes);
      if (!prop_res) {
        return NullOpt{};
      }

    } else if (tok.type == TokenType::keyword_methods) {
      auto method_res = methods_block(class_handle, method_names, methods, method_decls, method_def_nodes);
      if (!method_res) {
        return NullOpt{};
      }
    } else {
      std::array<TokenType, 2> possible_types{
        {TokenType::keyword_properties, TokenType::keyword_methods}
      };

      add_error(make_error_expected_token_type(parse_instance, tok, possible_types));
      return NullOpt{};
    }
  }

  auto err = consume(TokenType::keyword_end);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  ClassDef class_def(source_token, qualified_name, std::move(supers),
                     std::move(properties), std::move(methods));

  {
    Store::Write writer(*store);
    writer.emplace_definition(class_handle, std::move(class_def));
  }

  auto class_node = std::make_unique<ClassDefNode>(source_token, class_handle,
    std::move(property_nodes), std::move(method_def_nodes));

  bool register_success = parent_scope->register_class(qualified_name, class_handle);
  if (!register_success) {
    add_error(make_error_duplicate_class_def(parse_instance, source_token));
    return NullOpt{};
  }

  //  Add file entry class definition handle.
  parse_instance->set_file_entry_class_def(class_handle);

  const auto class_identifier = maybe_namespace_enclose_type_identifier(TypeIdentifier(qualified_name.full_name()));
  //  @Note: Class definition are always exported.
  const bool is_type_export = true;

  if (!parent_type_scope->can_register_local_identifier(class_identifier, is_type_export)) {
    add_error(make_error_duplicate_type_identifier(parse_instance, name_token));
    return NullOpt{};
  }

  auto& type_store = parse_instance->type_store;
  auto class_type = type_store->make_class(class_identifier, nullptr);
  auto class_ref = type_store->make_type_reference(&class_node->source_token, class_type, parent_type_scope);
  parent_type_scope->emplace_type(class_identifier, class_ref, is_type_export);

  //  Add pending method declarations.
  add_pending_external_methods(parse_instance, method_decls, class_handle, qualified_name,
                               class_node.get(), parent_scope, parent_type_scope);

  return Optional<BoxedAstNode>(std::move(class_node));
}

Optional<MatlabIdentifier> AstGenerator::meta_class() {
  iterator.advance(); //  consume ?
  auto component_res = compound_identifier_components();
  if (!component_res) {
    return NullOpt{};
  }

  const auto components = string_registry->register_strings(component_res.value());
  const auto identifier = string_registry->make_registered_compound_identifier(components);

  return Optional<MatlabIdentifier>(MatlabIdentifier(identifier, components.size()));
}

Optional<FunctionAttributes> AstGenerator::method_attributes(const ClassDefHandle& enclosing_class) {
  iterator.advance(); //  consume (

  FunctionAttributes attributes(enclosing_class);

  while (iterator.peek().type != TokenType::right_parens) {
    const auto& tok = iterator.peek();

    auto identifier_res = one_identifier();
    if (!identifier_res) {
      return NullOpt{};
    }

    const auto& attribute_name = identifier_res.value();

    if (!matlab::is_method_attribute(attribute_name)) {
      add_error(make_error_unrecognized_method_attribute(parse_instance, tok));
      return NullOpt{};
    }

    const bool is_boolean_attribute = matlab::is_boolean_method_attribute(attribute_name);
    const bool has_assigned_value = iterator.peek().type == TokenType::equal;

    if (is_boolean_attribute) {
      if (has_assigned_value) {
        //  (Static = true)
        const auto maybe_value = boolean_attribute_value();
        if (!maybe_value) {
          return NullOpt{};

        } else if (maybe_value.value()) {
          attributes.mark_boolean_attribute_from_name(attribute_name);
        }
      } else {
        //  (Static)
        attributes.mark_boolean_attribute_from_name(attribute_name);
      }
    } else if (matlab::is_access_attribute(attribute_name)) {
      //  (Access = public)
      if (!has_assigned_value) {
        //  Access must be explicitly set.
        add_error(consume(TokenType::equal).rvalue());
        return NullOpt{};

      } else {
        iterator.advance(); //  consume =

        auto access_res = access_specifier();
        if (!access_res) {
          return NullOpt{};
        } else {
          attributes.access_type = access_res.value().type;
        }
      }
    } else {
      assert(false && "Unhandled attribute.");
    }

    if (iterator.peek().type == TokenType::comma) {
      iterator.advance();
    }
  }

  auto err = consume(TokenType::right_parens);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  return Optional<FunctionAttributes>(attributes);
}

Optional<AccessSpecifier> AstGenerator::access_specifier() {
  auto type = AccessType::public_access;
  std::unique_ptr<MatlabIdentifier[]> classes = nullptr;
  int64_t num_classes = 0;

  const auto& tok = iterator.peek();
  if (tok.type == TokenType::left_brace) {
    //  @TODO: Access = {?class1, ?class2}
    const auto skip_to = TokenType::right_brace;
    iterator.advance_to_one(&skip_to, 1);
    auto err = consume(skip_to);
    if (err) {
      add_error(err.rvalue());
      return NullOpt{};
    }
  } else if (tok.type == TokenType::question) {
    //  Access = ?x
    auto meta_class_res = meta_class();
    if (!meta_class_res) {
      return NullOpt{};
    }
    type = AccessType::by_meta_classes_access;
    classes = std::unique_ptr<MatlabIdentifier[]>(new MatlabIdentifier[1]{meta_class_res.value()});
    num_classes = 1;

  } else {
    //  Access = public
    auto identifier_res = one_identifier();
    if (!identifier_res) {
      return NullOpt{};
    }

    const auto& attribute_value = identifier_res.value();

    if (!matlab::is_access_attribute_value(attribute_value)) {
      add_error(make_error_invalid_access_attribute_value(parse_instance, tok));
      return NullOpt{};
    } else {
      type = access_type_from_access_attribute_value(attribute_value);
    }
  }

  return Optional<AccessSpecifier>(AccessSpecifier(type, std::move(classes), num_classes));
}

Optional<bool> AstGenerator::boolean_attribute_value() {
  iterator.advance(); //  consume =

  auto identifier_res = one_identifier();
  if (!identifier_res) {
    return NullOpt{};
  }

  const auto& source_token = iterator.peek();
  const auto& value = identifier_res.value();

  if (!matlab::is_boolean(value)) {
    add_error(make_error_invalid_boolean_attribute_value(parse_instance, source_token));
    return NullOpt{};
  }

  return Optional<bool>(value == "true");
}

namespace {
  bool untyped_method_dispatch(AstGenerator* ast_gen,
                               const Token& tok,
                               std::set<int64_t>& method_names,
                               ClassDef::Methods& methods,
                               ClassDefNode::MethodDeclarations& method_decls,
                               BoxedMethodNodes& method_nodes,
                               bool* was_declaration) {
    if (tok.type == TokenType::identifier || tok.type == TokenType::left_bracket) {
      *was_declaration = true;
      return ast_gen->method_declaration(tok, method_names, method_decls);

    } else if (tok.type == TokenType::keyword_function) {
      *was_declaration = false;
      return ast_gen->method_def(tok, method_names, methods, method_nodes);

    } else {
      *was_declaration = false;

      std::array<TokenType, 3> possible_types{{
        TokenType::identifier, TokenType::left_bracket, TokenType::keyword_function
      }};

      ast_gen->add_error(make_error_expected_token_type(ast_gen->parse_instance, tok, possible_types));
      return false;
    }
  }

  bool typed_method_dispatch(AstGenerator* ast_gen,
                             std::set<int64_t>& method_names,
                             ClassDef::Methods& methods,
                             ClassDefNode::MethodDeclarations& method_decls,
                             BoxedMethodNodes& method_def_nodes) {
    ast_gen->iterator.advance();
    auto maybe_err = ast_gen->consume(TokenType::double_colon);
    if (maybe_err) {
      ast_gen->add_error(maybe_err.rvalue());
      return false;
    }

    auto type_res = ast_gen->type(ast_gen->iterator.peek(), /*allow_scheme=*/ true);
    if (!type_res) {
      return false;
    }

    auto end_type_err = ast_gen->consume(TokenType::keyword_end_type);
    if (end_type_err) {
      ast_gen->add_error(end_type_err.rvalue());
      return false;
    }

    bool was_decl;
    bool res = untyped_method_dispatch(ast_gen, ast_gen->iterator.peek(), method_names,
                                       methods, method_decls, method_def_nodes, &was_decl);
    if (!res) {
      return false;
    }

    if (was_decl) {
      assert(!method_decls.empty());
      auto& last_decl = method_decls.back();
      assert(!last_decl.maybe_type);
      last_decl.maybe_type = std::move(type_res.rvalue());
    } else {
      assert(!method_def_nodes.empty());
      auto& last_method = method_def_nodes.back();
      last_method->type = std::move(type_res.rvalue());
    }

    return true;
  }
}

bool AstGenerator::methods_block(const ClassDefHandle& enclosing_class,
                                 std::set<int64_t>& method_names,
                                 ClassDef::Methods& methods,
                                 ClassDefNode::MethodDeclarations& method_decls,
                                 BoxedMethodNodes& method_def_nodes) {
  iterator.advance(); //  consume methods

  //  methods (SetAccess = ...)
  if (iterator.peek().type == TokenType::left_parens) {
    auto attr_res = method_attributes(enclosing_class);
    if (!attr_res) {
      return false;
    }

    push_function_attributes(attr_res.value());
  } else {
    push_function_attributes(FunctionAttributes(enclosing_class));
  }

  //  Pop function attributes on return.
  FunctionAttributeHelper attr_helper(*this);

  while (iterator.has_next() && iterator.peek().type != TokenType::keyword_end) {
    const auto& tok = iterator.peek();

    switch (tok.type) {
      case TokenType::comma:
      case TokenType::semicolon:
      case TokenType::new_line:
        iterator.advance();
        break;
      case TokenType::type_annotation_macro: {
        bool success = typed_method_dispatch(this, method_names, methods,
                                             method_decls, method_def_nodes);
        if (!success) {
          return false;
        }
        break;
      }
      default: {
        bool ignore_was_decl;
        bool success = untyped_method_dispatch(this, tok, method_names, methods,
                                               method_decls, method_def_nodes, &ignore_was_decl);
        if (!success) {
          return false;
        }
      }
    }
  }

  auto err = consume(TokenType::keyword_end);
  if (err) {
    add_error(err.rvalue());
    return false;
  }

  return true;
}

bool AstGenerator::method_declaration(const mt::Token& source_token,
                                      std::set<int64_t>& method_names,
                                      ClassDefNode::MethodDeclarations& method_decls) {
  bool has_varargin;
  bool has_varargout;
  auto header_res = function_header(&has_varargin, &has_varargout);
  if (!header_res) {
    return false;
  }

  auto func_name = header_res.value().name;

  if (method_names.count(func_name.full_name()) > 0) {
    add_error(make_error_duplicate_method(parse_instance, source_token));
    return false;

  } else {
    method_names.emplace(func_name.full_name());
    FunctionDefHandle pending_def_handle;
    FunctionReferenceHandle pending_ref_handle;

    auto attrs = current_function_attributes();
    attrs.maybe_mark_has_varargin(has_varargin);
    attrs.maybe_mark_has_varargout(has_varargout);

    push_scope();
    MT_SCOPE_EXIT {
      pop_scope();
    };

    auto& header = header_res.value();
    auto curr_scope = current_scope();

    for (auto& arg : header.inputs) {
      arg.mark_is_part_of_decl();
    }
    for (auto& arg : header.outputs) {
      arg.mark_is_part_of_decl();
    }

    {
      Store::Write writer(*store);
      const auto name = header.name;

      pending_def_handle = writer.make_function_declaration(std::move(header), attrs);
      pending_ref_handle = writer.make_local_reference(name, pending_def_handle, curr_scope);
    }

    ClassDefNode::MethodDeclaration method_decl(pending_def_handle, pending_ref_handle, nullptr);
    method_decls.push_back(std::move(method_decl));
  }

  return true;
}

bool AstGenerator::method_def(const Token& source_token,
                              std::set<int64_t>& method_names,
                              mt::ClassDef::Methods& methods,
                              BoxedMethodNodes& method_nodes) {
  auto func_res = function_def();
  if (!func_res) {
    return false;
  }

  const auto& def_handle = func_res.value()->def_handle;
  Store::ReadConst reader(*store);
  const auto func_name = reader.at(def_handle).header.name;

  if (method_names.count(func_name.full_name()) > 0) {
    add_error(make_error_duplicate_method(parse_instance, source_token));
    return false;

  } else {
    method_names.emplace(func_name.full_name());
    methods.emplace_back(def_handle);

    auto untyped_node = std::make_unique<MethodNode>(func_res.rvalue(), nullptr, nullptr);
    method_nodes.push_back(std::move(untyped_node));
  }

  return true;
}

Optional<BoxedPropertyNode> AstGenerator::typed_property(const Token& source_token) {
  iterator.advance();
  auto type_assert_res = type_assertion(iterator.peek(),
                                        /*expect_enclosing_node=*/ true,
                                        /*allow_scheme=*/ false);
  if (!type_assert_res) {
    return NullOpt{};
  }

  auto& assert_ref = static_cast<TypeAssertion&>(*type_assert_res.value());
  if (!assert_ref.node->is_stmt()) {
    add_error(make_error_invalid_typed_property(parse_instance, source_token));
    return NullOpt{};
  }

  auto& stmt_node = static_cast<Stmt&>(*assert_ref.node);
  Optional<const IdentifierReferenceExpr*> property_lhs;
  BoxedExpr maybe_initializer;

  if (stmt_node.is_assignment_stmt()) {
    auto& assign_node = static_cast<AssignmentStmt&>(stmt_node);
    property_lhs = assign_node.to_expr->expect_identifier_reference_expr();
    maybe_initializer = std::move(assign_node.of_expr);

  } else if (stmt_node.is_expr_stmt()) {
    auto& expr_node = static_cast<ExprStmt&>(stmt_node);
    property_lhs = expr_node.expr->expect_identifier_reference_expr();
  }

  if (!property_lhs || !property_lhs.value()->is_non_subscripted_scalar_reference()) {
    add_error(make_error_invalid_typed_property(parse_instance, source_token));
    return NullOpt{};
  }

  auto type = std::move(assert_ref.has_type);
  auto name = property_lhs.value()->primary_identifier;
  auto prop_node = std::make_unique<PropertyNode>(source_token, name, std::move(maybe_initializer), std::move(type));

  return Optional<BoxedPropertyNode>(std::move(prop_node));
}


bool AstGenerator::emplace_property(const Token& source_token,
                                    BoxedPropertyNode property_node,
                                    std::set<int64_t>& property_names,
                                    std::vector<ClassDef::Property>& properties,
                                    BoxedPropertyNodes& property_nodes) {

  const auto& prop_name = property_node->name;

  if (property_names.count(prop_name.full_name()) > 0) {
    //  Duplicate property
    add_error(make_error_duplicate_class_property(parse_instance, source_token));
    return false;
  } else {
    property_names.emplace(prop_name.full_name());
    properties.emplace_back(ClassDef::Property(prop_name));
    property_nodes.push_back(std::move(property_node));
  }

  return true;
}

bool AstGenerator::properties_block(std::set<int64_t>& property_names,
                                    std::vector<ClassDef::Property>& properties,
                                    BoxedPropertyNodes& property_nodes) {
  iterator.advance(); //  consume properties

  //  properties (SetAccess = ...)
  if (iterator.peek().type == TokenType::left_parens) {
    //  @TODO: Parse attributes
    auto right_parens = TokenType::right_parens;
    iterator.advance_to_one(&right_parens, 1);
    auto err = consume(right_parens);
    if (err) {
      add_error(err.rvalue());
      return false;
    }
  }

  while (iterator.has_next() && iterator.peek().type != TokenType::keyword_end) {
    const auto& tok = iterator.peek();

    switch (tok.type) {
      case TokenType::comma:
      case TokenType::semicolon:
      case TokenType::new_line:
        iterator.advance();
        break;
      case TokenType::type_annotation_macro: {
        auto typed_prop_res = typed_property(iterator.peek());
        if (!typed_prop_res) {
          return false;
        }
        auto prop_node = std::move(typed_prop_res.rvalue());
        bool success = emplace_property(tok, std::move(prop_node), property_names, properties, property_nodes);
        if (!success) {
          return false;
        }
        break;
      }
      default: {
        auto prop_res = property(iterator.peek());
        if (!prop_res) {
          return false;
        }
        auto property_node = std::move(prop_res.rvalue());
        bool success = emplace_property(tok, std::move(property_node), property_names, properties, property_nodes);
        if (!success) {
          return false;
        }
      }
    }
  }

  auto err = consume(TokenType::keyword_end);
  if (err) {
    add_error(err.rvalue());
    return false;
  }

  return true;
}

Optional<BoxedPropertyNode> AstGenerator::property(const Token& source_token) {
  auto identifier_res = one_identifier();
  if (!identifier_res) {
    return NullOpt{};
  }

  BoxedExpr initializer;

  if (iterator.peek().type == TokenType::equal) {
    iterator.advance();
    auto initializer_expr = expr();
    if (!initializer_expr) {
      return NullOpt{};
    } else {
      initializer = initializer_expr.rvalue();
    }
  }

  auto name = MatlabIdentifier(string_registry->register_string(identifier_res.value()));
  //  No type.
  auto node = std::make_unique<PropertyNode>(source_token, name, std::move(initializer), nullptr);

  return Optional<BoxedPropertyNode>(std::move(node));
}

Optional<std::unique_ptr<Block>> AstGenerator::block() {
  bool should_proceed = true;
  bool any_error = false;

  std::vector<BoxedAstNode> functions;
  std::vector<BoxedAstNode> non_functions;

  while (iterator.has_next() && should_proceed) {
    BoxedAstNode node;
    bool success = true;
    bool is_function = false;

    const auto& tok = iterator.peek();

    switch (tok.type) {
      case TokenType::new_line:
      case TokenType::null:
      case TokenType::semicolon:
      case TokenType::comma:
        iterator.advance();
        break;
      case TokenType::keyword_end:
        should_proceed = false;
        break;
      case TokenType::type_annotation_macro: {
        auto type_res = type_annotation_macro(tok);
        if (type_res) {
          node = type_res.rvalue();
        } else {
          success = false;
        }
        break;
      }
      case TokenType::keyword_classdef: {
        auto def_res = class_def();
        if (def_res) {
          node = def_res.rvalue();
        } else {
          success = false;
        }
        break;
      }
      case TokenType::keyword_function: {
        auto def_res = function_def();
        if (def_res) {
          node = def_res.rvalue();
          is_function = true;
        } else {
          success = false;
        }
        break;
      }
      default: {
        auto stmt_res = stmt();
        if (stmt_res) {
          node = stmt_res.rvalue();
        } else {
          success = false;
        }
        //  If the first statement precedes a function or class definition, we know this file is
        //  a script.
        if (!parse_instance->registered_file_type) {
          parse_instance->register_file_type(CodeFileType::script_with_local_functions);
        }
      }
    }

    if (!success) {
      any_error = true;
      std::array<TokenType, 1> possible_types{{TokenType::keyword_function}};
      iterator.advance_to_one(possible_types.data(), possible_types.size());

    } else if (node) {
      if (is_function) {
        functions.emplace_back(std::move(node));
      } else {
        non_functions.emplace_back(std::move(node));
      }
    }
  }

  if (any_error) {
    return NullOpt{};
  } else {
    auto block_node = std::make_unique<Block>();
    block_node->append_many(non_functions);
    block_node->append_many(functions);
    return Optional<std::unique_ptr<Block>>(std::move(block_node));
  }
}

Optional<std::unique_ptr<Block>> AstGenerator::sub_block() {
  bool should_proceed = true;
  bool any_error = false;

  std::vector<BoxedAstNode> functions;
  std::vector<BoxedAstNode> non_functions;

  while (iterator.has_next() && should_proceed) {
    BoxedAstNode node;
    bool success = true;
    bool is_function = false;

    const auto& tok = iterator.peek();

    switch (tok.type) {
      case TokenType::new_line:
      case TokenType::null:
      case TokenType::semicolon:
      case TokenType::comma:
        iterator.advance();
        break;
      case TokenType::keyword_end:
      case TokenType::keyword_else:
      case TokenType::keyword_elseif:
      case TokenType::keyword_catch:
      case TokenType::keyword_case:
      case TokenType::keyword_otherwise:
        should_proceed = false;
        break;
      case TokenType::type_annotation_macro: {
        auto type_res = type_annotation_macro(tok);
        if (type_res) {
          node = type_res.rvalue();
        } else {
          success = false;
        }
        break;
      }
      case TokenType::keyword_function: {
        if (parse_instance->functions_are_end_terminated) {
          auto def_res = function_def();
          if (def_res) {
            node = def_res.rvalue();
            is_function = true;
          } else {
            success = false;
          }
        } else {
          //  In a non-end terminated function, a function keyword here is actually the start of a
          //  new function at sibling-scope.
          should_proceed = false;
        }
        break;
      }
      default: {
        auto stmt_res = stmt();
        if (stmt_res) {
          node = stmt_res.rvalue();
        } else {
          success = false;
        }
      }
    }

    if (!success) {
      any_error = true;
      const auto possible_types = sub_block_possible_types();
      iterator.advance_to_one(possible_types.data(), possible_types.size());

    } else if (node) {
      if (is_function) {
        functions.emplace_back(std::move(node));
      } else {
        non_functions.emplace_back(std::move(node));
      }
    }
  }

  if (any_error) {
    return NullOpt{};
  } else {
    auto block_node = std::make_unique<Block>();
    block_node->append_many(non_functions);
    block_node->append_many(functions);
    return Optional<std::unique_ptr<Block>>(std::move(block_node));
  }
}

Optional<BoxedExpr> AstGenerator::literal_field_reference_expr(const Token& source_token) {
  //  Of the form a.b, where `b` is the identifier representing a presumed field of `a`.
  auto ident_res = one_identifier();
  if (!ident_res) {
    return NullOpt{};
  }

  int64_t field_identifier = string_registry->register_string(ident_res.value());
  auto expr = std::make_unique<LiteralFieldReferenceExpr>(source_token, field_identifier);
  return Optional<BoxedExpr>(std::move(expr));
}

Optional<BoxedExpr> AstGenerator::dynamic_field_reference_expr(const Token& source_token) {
  //  Of the form a.(`expr`), where `expr` is assumed to evaluate to a field of `a`.
  iterator.advance(); //  consume (

  auto expr_res = expr();
  if (!expr_res) {
    return NullOpt{};
  }

  auto err = consume(TokenType::right_parens);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  auto expr = std::make_unique<DynamicFieldReferenceExpr>(source_token, expr_res.rvalue());
  return Optional<BoxedExpr>(std::move(expr));
}

Optional<Subscript> AstGenerator::non_period_subscript(const Token& source_token,
                                                       mt::SubscriptMethod method,
                                                       mt::TokenType term) {
  iterator.advance(); //  consume '(' or '{'
  std::vector<BoxedExpr> args;

  while (iterator.has_next() && iterator.peek().type != term) {
    auto expr_res = expr();
    if (!expr_res) {
      return NullOpt{};
    }

    args.emplace_back(expr_res.rvalue());

    if (iterator.peek().type == TokenType::comma) {
      iterator.advance();
    } else {
      break;
    }
  }

  auto err = consume(term);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  if (method == SubscriptMethod::brace && args.empty()) {
    add_error(make_error_empty_brace_subscript(parse_instance, source_token));
    return NullOpt{};
  }

  Subscript subscript_expr(source_token, method, std::move(args));
  return Optional<Subscript>(std::move(subscript_expr));
}

Optional<Subscript> AstGenerator::period_subscript(const Token& source_token) {
  iterator.advance(); //  consume '.'

  const auto& tok = iterator.peek();
  Optional<BoxedExpr> expr_res;

  switch (tok.type) {
    case TokenType::identifier:
      expr_res = literal_field_reference_expr(tok);
      break;
    case TokenType::left_parens:
      expr_res = dynamic_field_reference_expr(tok);
      break;
    default:
      std::array<TokenType, 2> possible_types{{TokenType::identifier, TokenType::left_parens}};
      add_error(make_error_expected_token_type(parse_instance, tok, possible_types));
      return NullOpt{};
  }

  if (!expr_res) {
    return NullOpt{};
  }

  std::vector<BoxedExpr> args;
  args.emplace_back(expr_res.rvalue());

  Subscript subscript_expr(source_token, SubscriptMethod::period, std::move(args));
  return Optional<Subscript>(std::move(subscript_expr));
}

Optional<BoxedExpr> AstGenerator::identifier_reference_expr(const Token& source_token) {
  auto main_ident_res = one_identifier();
  if (!main_ident_res) {
    return NullOpt{};
  }

  std::vector<Subscript> subscripts;
  SubscriptMethod prev_method = SubscriptMethod::unknown;

  while (iterator.has_next()) {
    const auto& tok = iterator.peek();

    Optional<Subscript> sub_result = NullOpt{};
    bool has_subscript = true;

    if (tok.type == TokenType::period) {
      sub_result = period_subscript(tok);

    } else if (tok.type == TokenType::left_parens) {
      sub_result = non_period_subscript(tok, SubscriptMethod::parens, TokenType::right_parens);

    } else if (tok.type == TokenType::left_brace) {
      sub_result = non_period_subscript(tok, SubscriptMethod::brace, TokenType::right_brace);

    } else {
      has_subscript = false;
    }

    if (!has_subscript) {
      //  This is a simple variable reference or function call with no subscripts, e.g. `a;`
      break;
    } else if (!sub_result) {
      return NullOpt{};
    }

    auto sub_expr = sub_result.rvalue();

    if (prev_method == SubscriptMethod::parens && sub_expr.method != SubscriptMethod::period) {
      //  a(1, 1)(1) is an error, but a.('x')(1) is allowed.
      add_error(make_error_reference_after_parens_reference_expr(parse_instance, tok));
      return NullOpt{};
    } else {
      prev_method = sub_expr.method;
      subscripts.emplace_back(std::move(sub_expr));
    }
  }

  //  Register main identifier.
  const int64_t primary_identifier_id = string_registry->register_string(main_ident_res.value());
  MatlabIdentifier primary_identifier(primary_identifier_id);

  auto node = std::make_unique<IdentifierReferenceExpr>(source_token, primary_identifier, std::move(subscripts));
  return Optional<BoxedExpr>(std::move(node));
}

Optional<BoxedExpr> AstGenerator::function_reference_expr(const Token& source_token) {
  //  @main
  iterator.advance(); //  consume '@'

  auto component_res = compound_identifier_components();
  if (!component_res) {
    return NullOpt{};
  }

  const auto component_identifiers = string_registry->register_strings(component_res.value());
  const auto compound_identifier = string_registry->make_registered_compound_identifier(component_identifiers);
  MatlabIdentifier identifier(compound_identifier, component_identifiers.size());

  auto node = std::make_unique<FunctionReferenceExpr>(source_token, identifier);
  return Optional<BoxedExpr>(std::move(node));
}

Optional<BoxedExpr> AstGenerator::anonymous_function_expr(const mt::Token& source_token) {
  iterator.advance(); //  consume '@'

  //  Expect @(x, y, z) expr
  auto parens_err = consume(TokenType::left_parens);
  if (parens_err) {
    add_error(parens_err.rvalue());
    return NullOpt{};
  }

  ParseScopeHelper scope_helper(*this);
  auto scope = current_scope();
  auto type_scope = current_type_scope();
  bool has_varargin;

  auto input_res = function_parameters(&has_varargin);
  if (!input_res) {
    return NullOpt{};
  }

  //  @Hack: Expect artificially-inserted comma to mark the beginning of the anonymous function
  //  expression. This is necessary to correctly parse expressions that begin with the prefix unary
  //  operators +/-, e.g. in @(a) -1; Otherwise, the right parens would cause `expr()` to parse the
  //  operator as part of a binary expression. See token_manipulation
  auto err = consume(TokenType::comma);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  auto body_res = expr();
  if (!body_res) {
    return NullOpt{};
  }

  auto input_identifiers = std::move(input_res.rvalue());

  auto params_err = check_parameters_are_unique(source_token, input_identifiers);
  if (params_err) {
    add_error(params_err.rvalue());
    return NullOpt{};
  }

  auto node = std::make_unique<AnonymousFunctionExpr>(source_token, std::move(input_identifiers),
                                                      body_res.rvalue(), scope, type_scope);
  return Optional<BoxedExpr>(std::move(node));
}

Optional<BoxedExpr> AstGenerator::grouping_expr(const Token& source_token) {
  const TokenType terminator = grouping_terminator_for(source_token.type);
  iterator.advance();

  //  Concatenation constructions with brackets and braces can have empty expressions, e.g.
  //  [,,,,,,,,,;;;] and {,,,,,;;;;;} are valid and equivalent to [] and {}.
  const auto source_type = source_token.type;
  const bool allow_empty = source_type == TokenType::left_bracket || source_type == TokenType::left_brace;

  std::vector<GroupingExprComponent> exprs;

  while (iterator.has_next() && iterator.peek().type != terminator) {
    auto expr_res = expr(allow_empty);
    if (!expr_res) {
      return NullOpt{};
    }

    const auto& next = iterator.peek();
    TokenType delim = TokenType::comma;

    if (next.type == TokenType::comma) {
      iterator.advance();

    } else if (next.type == TokenType::semicolon || next.type == TokenType::new_line) {
      iterator.advance();
      delim = TokenType::semicolon;

    } else if (next.type != terminator) {
      std::array<TokenType, 4> possible_types{
        {TokenType::comma, TokenType::semicolon, TokenType::new_line, terminator}
      };
      add_error(make_error_expected_token_type(parse_instance, next, possible_types));
      return NullOpt{};
    }

    if (source_token.type == TokenType::left_parens && next.type != terminator) {
      //  (1, 2) or (1; 3) is not a valid expression.
      add_error(make_error_multiple_exprs_in_parens_grouping_expr(parse_instance, next));
      return NullOpt{};
    }

    if (expr_res.value()) {
      //  If non-empty.
      GroupingExprComponent grouping_component(expr_res.rvalue(), delim);
      exprs.emplace_back(std::move(grouping_component));
    }
  }

  auto err = consume(terminator);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  const auto grouping_method = grouping_method_from_token_type(source_token.type);
  auto expr_node = std::make_unique<GroupingExpr>(source_token, grouping_method, std::move(exprs));
  return Optional<BoxedExpr>(std::move(expr_node));
}

Optional<BoxedExpr> AstGenerator::colon_subscript_expr(const mt::Token& source_token) {
  iterator.advance(); //  consume ':'

  auto node = std::make_unique<ColonSubscriptExpr>(source_token);
  return Optional<BoxedExpr>(std::move(node));
}

Optional<BoxedExpr> AstGenerator::literal_expr(const Token& source_token) {
  iterator.advance();
  BoxedExpr expr_node;

  if (source_token.type == TokenType::number_literal) {
    const double num = std::stod(source_token.lexeme.data());
    expr_node = std::make_unique<NumberLiteralExpr>(source_token, num);

  } else if (source_token.type == TokenType::char_literal) {
    expr_node = std::make_unique<CharLiteralExpr>(source_token);

  } else if (source_token.type == TokenType::string_literal) {
    expr_node = std::make_unique<StringLiteralExpr>(source_token);

  } else {
    assert(false && "Unhandled literal expression.");
  }

  return Optional<BoxedExpr>(std::move(expr_node));
}

Optional<BoxedExpr> AstGenerator::ignore_argument_expr(const Token& source_token) {
  //  [~, a] = b(), or @(x, ~, z) y();
  iterator.advance();

  auto source_node = std::make_unique<IgnoreFunctionArgumentExpr>(source_token);
  return Optional<BoxedExpr>(std::move(source_node));
}

std::unique_ptr<UnaryOperatorExpr> AstGenerator::pending_unary_prefix_expr(const mt::Token& source_token) {
  iterator.advance();
  const auto op = unary_operator_from_token_type(source_token.type);
  //  Awaiting expr.
  return std::make_unique<UnaryOperatorExpr>(source_token, op, nullptr);
}

Optional<ParseError> AstGenerator::postfix_unary_expr(const mt::Token& source_token,
                                                      std::vector<BoxedExpr>& completed) {
  iterator.advance();

  if (completed.empty()) {
    return Optional<ParseError>(make_error_expected_lhs(parse_instance, source_token));
  }

  const auto op = unary_operator_from_token_type(source_token.type);
  auto node = std::make_unique<UnaryOperatorExpr>(source_token, op, std::move(completed.back()));
  completed.back() = std::move(node);

  return NullOpt{};
}

Optional<ParseError> AstGenerator::handle_postfix_unary_exprs(std::vector<BoxedExpr>& completed) {
  while (iterator.has_next()) {
    const auto& curr = iterator.peek();
    const auto& prev = iterator.peek_prev();

    if (represents_postfix_unary_operator(curr.type) && can_precede_postfix_unary_operator(prev.type)) {
      auto err = postfix_unary_expr(curr, completed);
      if (err) {
        return err;
      }
    } else {
      break;
    }
  }

  return NullOpt{};
}

void AstGenerator::handle_prefix_unary_exprs(std::vector<BoxedExpr>& completed,
                                             std::vector<BoxedUnaryOperatorExpr>& unaries) {
  while (!unaries.empty()) {
    auto un = std::move(unaries.back());
    unaries.pop_back();
    assert(!un->expr && "Unary already had an expression.");
    un->expr = std::move(completed.back());
    completed.back() = std::move(un);
  }
}

void AstGenerator::handle_binary_exprs(std::vector<BoxedExpr>& completed,
                                       std::vector<BoxedBinaryOperatorExpr>& binaries,
                                       std::vector<BoxedBinaryOperatorExpr>& pending_binaries) {
  assert(!binaries.empty() && "Binaries were empty.");
  auto bin = std::move(binaries.back());
  binaries.pop_back();

  auto prec_curr = precedence(bin->op);
  const auto op_next = binary_operator_from_token_type(iterator.peek().type);
  const auto prec_next = precedence(op_next);

  if (prec_curr < prec_next) {
    pending_binaries.emplace_back(std::move(bin));
  } else {
    assert(!bin->right && "Binary already had an expression.");
    auto complete = std::move(completed.back());
    bin->right = std::move(complete);
    completed.back() = std::move(bin);

    while (!pending_binaries.empty() && prec_next < prec_curr) {
      auto pend = std::move(pending_binaries.back());
      pending_binaries.pop_back();
      prec_curr = precedence(pend->op);
      pend->right = std::move(completed.back());
      completed.back() = std::move(pend);
    }
  }
}

Optional<ParseError> AstGenerator::pending_binary_expr(const mt::Token& source_token,
                                                       std::vector<BoxedExpr>& completed,
                                                       std::vector<BoxedBinaryOperatorExpr>& binaries) {
  iterator.advance(); //  Consume operator token.

  if (completed.empty()) {
    return Optional<ParseError>(make_error_expected_lhs(parse_instance, source_token));
  }

  auto left = std::move(completed.back());
  completed.pop_back();

  const auto op = binary_operator_from_token_type(source_token.type);
  auto pending = std::make_unique<BinaryOperatorExpr>(source_token, op, std::move(left), nullptr);
  binaries.emplace_back(std::move(pending));

  return NullOpt{};
}

Optional<BoxedExpr> AstGenerator::function_expr(const Token& source_token) {
  if (iterator.peek_next().type == TokenType::identifier) {
    return function_reference_expr(source_token);
  } else {
    return anonymous_function_expr(source_token);
  }
}

Optional<BoxedExpr> AstGenerator::presumed_superclass_method_reference_expr(const Token& source_token,
                                                                            BoxedExpr method_reference_expr) {
  //  @Note: This function is called for the syntax identifier@function_call(1, 2, 3), where
  //  `identifier` is either a) the name of a superclass method or b) the name bound to the object
  //  instance in a constructor (e.g., obj@Superclass(args)). We ensure that `identifier` and
  //  `function_call` are identifier reference expressions, and then convert the invoking expression
  //  to a plain identifier, implicitly deleting `method_reference_expr` when this function returns.
  iterator.advance(); //  consume @

  auto superclass_res = expr();
  if (!superclass_res) {
    return NullOpt{};
  }

  auto superclass_method_expr = std::move(superclass_res.rvalue());

  if (!method_reference_expr->is_static_identifier_reference_expr() ||
      !superclass_method_expr->is_identifier_reference_expr()) {
    //
    add_error(make_error_invalid_superclass_method_reference_expr(parse_instance, source_token));
    return NullOpt{};
  }

  auto* method_expr = static_cast<IdentifierReferenceExpr*>(method_reference_expr.get());

  int64_t subscript_end;
  const auto compound_identifier_components = method_expr->make_compound_identifier(&subscript_end);

  auto compound_identifier = string_registry->make_registered_compound_identifier(compound_identifier_components);
  MatlabIdentifier invoking_argument(compound_identifier);

  auto node = std::make_unique<SuperclassMethodReferenceExpr>(source_token,
                                                              invoking_argument, std::move(superclass_method_expr));

  return Optional<BoxedExpr>(std::move(node));
}

Optional<BoxedExpr> AstGenerator::expr(bool allow_empty) {
  std::vector<BoxedBinaryOperatorExpr> pending_binaries;
  std::vector<BoxedBinaryOperatorExpr> binaries;
  std::vector<BoxedUnaryOperatorExpr> unaries;
  std::vector<BoxedExpr> completed;

  while (iterator.has_next()) {
    Optional<BoxedExpr> node_res(nullptr);
    const auto& tok = iterator.peek();

    if (tok.type == TokenType::identifier) {
      node_res = identifier_reference_expr(tok);
      if (node_res && iterator.peek().type == TokenType::at) {
        //  method@superclass()
        node_res = presumed_superclass_method_reference_expr(iterator.peek(), std::move(node_res.rvalue()));
      }

    } else if (is_ignore_argument_expr(tok)) {
      node_res = ignore_argument_expr(tok);

    } else if (represents_grouping_initiator(tok.type)) {
      node_res = grouping_expr(tok);

    } else if (represents_literal(tok.type)) {
      node_res = literal_expr(tok);

    } else if (tok.type == TokenType::op_end) {
      iterator.advance();
      node_res = Optional<BoxedExpr>(std::make_unique<EndOperatorExpr>(tok));

    } else if (is_unary_prefix_expr(tok)) {
      unaries.emplace_back(pending_unary_prefix_expr(tok));

    } else if (represents_binary_operator(tok.type)) {
      if (is_colon_subscript_expr(tok)) {
        node_res = colon_subscript_expr(tok);
      } else {
        //  @TODO: Tertiary colon operator is not parsed correctly. E.g., 1:2:3:4:5:6 should parse
        //  as (((1:2:3):4:5):6)
        auto bin_res = pending_binary_expr(tok, completed, binaries);
        if (bin_res) {
          add_error(bin_res.rvalue());
          return NullOpt{};
        }
      }

    } else if (tok.type == TokenType::at) {
      node_res = function_expr(tok);

    } else if (represents_expr_terminator(tok.type)) {
      break;

    } else {
      add_error(make_error_invalid_expr_token(parse_instance, tok));
      return NullOpt{};
    }

    if (!node_res) {
      //  An error occurred in one of the sub-expressions.
      return NullOpt{};
    }

    if (node_res.value()) {
      completed.emplace_back(node_res.rvalue());

      auto err = handle_postfix_unary_exprs(completed);
      if (err) {
        add_error(err.rvalue());
        return NullOpt{};
      }
    }

    if (!completed.empty()) {
      handle_prefix_unary_exprs(completed, unaries);
    }

    if (!completed.empty() && !binaries.empty()) {
      handle_binary_exprs(completed, binaries, pending_binaries);
    }
  }

  if (completed.size() == 1) {
    return Optional<BoxedExpr>(std::move(completed.back()));

  } else {
    const bool is_complete = completed.empty() && pending_binaries.empty() && binaries.empty() && unaries.empty();
    const bool is_valid = is_complete && allow_empty;

    if (!is_valid) {
      add_error(make_error_incomplete_expr(parse_instance, iterator.peek()));
      return NullOpt{};
    } else {
      return Optional<BoxedExpr>(nullptr);
    }
  }
}

Optional<BoxedStmt> AstGenerator::control_stmt(const Token& source_token) {
  iterator.advance();

  const auto kind = control_flow_manipulator_from_token_type(source_token.type);
  if (is_loop_control_flow_manipulator(kind) && !is_within_loop()) {
    add_error(make_error_loop_control_flow_manipulator_outside_loop(parse_instance, source_token));
    return NullOpt{};
  }

  auto node = std::make_unique<ControlStmt>(source_token, kind);

  return Optional<BoxedStmt>(std::move(node));
}

Optional<SwitchCase> AstGenerator::switch_case(const mt::Token& source_token) {
  iterator.advance();

  auto condition_expr_res = expr();
  if (!condition_expr_res) {
    return NullOpt{};
  }

  auto block_res = sub_block();
  if (!block_res) {
    return NullOpt{};
  }

  SwitchCase switch_case(source_token, condition_expr_res.rvalue(), block_res.rvalue());
  return Optional<SwitchCase>(std::move(switch_case));
}

Optional<BoxedStmt> AstGenerator::switch_stmt(const mt::Token& source_token) {
  iterator.advance();

  //  Register an increase in the switch statement scope depth.
  BlockStmtScopeHelper scope_helper(&block_depths.switch_stmt);

  auto condition_expr_res = expr();
  if (!condition_expr_res) {
    return NullOpt{};
  }

  std::vector<SwitchCase> cases;
  BoxedBlock otherwise;

  while (iterator.has_next() && iterator.peek().type != TokenType::keyword_end) {
    const auto& tok = iterator.peek();

    switch (tok.type) {
      case TokenType::new_line:
      case TokenType::comma:
      case TokenType::semicolon:
        iterator.advance();
        break;
      case TokenType::keyword_case: {
        auto case_res = switch_case(tok);
        if (case_res) {
          cases.emplace_back(case_res.rvalue());
        } else {
          return NullOpt{};
        }
        break;
      }
      case TokenType::keyword_otherwise: {
        iterator.advance();

        if (otherwise) {
          //  Duplicate otherwise in switch statement.
          add_error(make_error_duplicate_otherwise_in_switch_stmt(parse_instance, tok));
          return NullOpt{};
        }

        auto block_res = sub_block();
        if (!block_res) {
          return NullOpt{};
        } else {
          otherwise = block_res.rvalue();
        }
        break;
      }
      default: {
        std::array<TokenType, 2> possible_types{{TokenType::keyword_case, TokenType::keyword_otherwise}};
        add_error(make_error_expected_token_type(parse_instance, tok, possible_types));
        return NullOpt{};
      }
    }
  }

  auto err = consume(TokenType::keyword_end);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  auto node = std::make_unique<SwitchStmt>(source_token,
    condition_expr_res.rvalue(), std::move(cases), std::move(otherwise));

  return Optional<BoxedStmt>(std::move(node));
}

Optional<BoxedStmt> AstGenerator::while_stmt(const Token& source_token) {
  iterator.advance();

  //  Register an increase in the while statement scope depth.
  BlockStmtScopeHelper scope_helper(&block_depths.while_stmt);

  auto loop_condition_expr_res = expr();
  if (!loop_condition_expr_res) {
    return NullOpt{};
  }

  auto body_res = sub_block();
  if (!body_res) {
    return NullOpt{};
  }

  auto err = consume(TokenType::keyword_end);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  auto node = std::make_unique<WhileStmt>(source_token,
    loop_condition_expr_res.rvalue(), body_res.rvalue());

  return Optional<BoxedStmt>(std::move(node));
}

Optional<BoxedStmt> AstGenerator::for_stmt(const Token& source_token) {
  iterator.advance();

  //  Register an increase in the for statement scope depth.
  BlockStmtScopeHelper scope_helper(&block_depths.for_stmt);

  //  for (i = 1:10)
  const bool is_wrapped_by_parens = iterator.peek().type == TokenType::left_parens;
  if (is_wrapped_by_parens) {
    iterator.advance();
  }

  auto loop_var_res = one_identifier();
  if (!loop_var_res) {
    return NullOpt{};
  }

  auto eq_err = consume(TokenType::equal);
  if (eq_err) {
    add_error(eq_err.rvalue());
    return NullOpt{};
  }

  auto initializer_res = expr();
  if (!initializer_res) {
    return NullOpt{};
  }

  if (is_wrapped_by_parens) {
    auto parens_err = consume(TokenType::right_parens);
    if (parens_err) {
      return NullOpt{};
    }
  }

  auto block_res = sub_block();
  if (!block_res) {
    return NullOpt{};
  }

  auto end_err = consume(TokenType::keyword_end);
  if (end_err) {
    add_error(end_err.rvalue());
    return NullOpt{};
  }

  MatlabIdentifier loop_variable_id = MatlabIdentifier(string_registry->register_string(loop_var_res.value()));

  auto node = std::make_unique<ForStmt>(source_token, loop_variable_id,
    initializer_res.rvalue(), block_res.rvalue());

  return Optional<BoxedStmt>(std::move(node));
}

Optional<BoxedStmt> AstGenerator::if_stmt(const Token& source_token) {
  //  Register an increase in the if statement scope depth.
  BlockStmtScopeHelper scope_helper(&block_depths.if_stmt);

  auto main_branch_res = if_branch(source_token);
  if (!main_branch_res) {
    return NullOpt{};
  }

  std::vector<IfBranch> elseif_branches;

  while (iterator.peek().type == TokenType::keyword_elseif) {
    auto sub_branch_res = if_branch(iterator.peek());
    if (!sub_branch_res) {
      return NullOpt{};
    } else {
      elseif_branches.emplace_back(sub_branch_res.rvalue());
    }
  }

  Optional<ElseBranch> else_branch = NullOpt{};
  const auto& maybe_else_token = iterator.peek();

  if (maybe_else_token.type == TokenType::keyword_else) {
    iterator.advance();

    auto else_block_res = sub_block();
    if (!else_block_res) {
      return NullOpt{};
    } else {
      else_branch = ElseBranch(maybe_else_token, else_block_res.rvalue());
    }
  }

  auto err = consume(TokenType::keyword_end);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  auto if_stmt_node = std::make_unique<IfStmt>(main_branch_res.rvalue(),
    std::move(elseif_branches), std::move(else_branch));

  return Optional<BoxedStmt>(std::move(if_stmt_node));
}

Optional<IfBranch> AstGenerator::if_branch(const Token& source_token) {
  //  @TODO: Handle one line branch if (condition) d = 10; end
  iterator.advance(); //  consume if

  auto condition_res = expr();
  if (!condition_res) {
    return NullOpt{};
  }

  auto block_res = sub_block();
  if (!block_res) {
    return NullOpt{};
  }

  IfBranch if_branch(source_token, condition_res.rvalue(), block_res.rvalue());
  return Optional<IfBranch>(std::move(if_branch));
}

Optional<BoxedStmt> AstGenerator::assignment_stmt(BoxedExpr lhs, const Token& initial_token) {
  iterator.advance(); //  consume =

  if (!lhs->is_valid_assignment_target()) {
    add_error(make_error_invalid_assignment_target(parse_instance, initial_token));
    return NullOpt{};
  }

  auto rhs_res = expr();
  if (!rhs_res) {
    return NullOpt{};
  }

  auto assign_stmt = std::make_unique<AssignmentStmt>(std::move(lhs), rhs_res.rvalue());
  return Optional<BoxedStmt>(std::move(assign_stmt));
}

Optional<BoxedStmt> AstGenerator::variable_declaration_stmt(const Token& source_token) {
  iterator.advance();

  bool should_proceed = true;
  std::vector<MatlabIdentifier> identifiers;

  while (iterator.has_next() && should_proceed) {
    const auto& tok = iterator.peek();

    switch (tok.type) {
      case TokenType::null:
      case TokenType::new_line:
      case TokenType::semicolon:
      case TokenType::comma:
        should_proceed = false;
        break;
      case TokenType::identifier:
        iterator.advance();
        identifiers.push_back(MatlabIdentifier(string_registry->register_string(tok.lexeme)));
        break;
      default:
        //  e.g. global 1
        iterator.advance();
        const auto expected_type = TokenType::identifier;
        add_error(make_error_expected_token_type(parse_instance, tok, &expected_type, 1));
        return NullOpt{};
    }
  }

  const auto var_qualifier = variable_declaration_qualifier_from_token_type(source_token.type);
  auto node = std::make_unique<VariableDeclarationStmt>(source_token, var_qualifier, std::move(identifiers));
  return Optional<BoxedStmt>(std::move(node));
}

Optional<BoxedStmt> AstGenerator::import_stmt(const mt::Token&) {
  iterator.advance();

  while (iterator.has_next()) {
    auto import_res = one_import(iterator.peek(), /*is_matlab_import=*/ true);
    if (!import_res) {
      return NullOpt{};
    }

    register_import(import_res.rvalue());

    if (represents_stmt_terminator(iterator.peek().type)) {
      break;
    }
  }

  return Optional<BoxedStmt>(BoxedStmt(nullptr));
}

Optional<Import> AstGenerator::one_import(const mt::Token& source_token, bool is_matlab_import) {
  std::vector<int64_t> identifier_components;
  bool expect_identifier = true;
  ImportType import_type = ImportType::fully_qualified;

  while (iterator.has_next() && !represents_stmt_terminator(iterator.peek().type)) {
    const auto& tok = iterator.peek();
    const auto expect_type = expect_identifier ? TokenType::identifier : TokenType::period;

    if (tok.type != expect_type) {
      if (tok.type == TokenType::identifier) {
        //  Start of a new import. e.g. import a.b.c b.c.d
        break;
      } else if (tok.type == TokenType::dot_asterisk && is_matlab_import) {
        //  Wildcard import a.b.c*
        iterator.advance();
        import_type = ImportType::wildcard;
        break;
      } else {
        std::array<TokenType, 2> possible_types{{TokenType::identifier, TokenType::period}};
        add_error(make_error_expected_token_type(parse_instance, tok, possible_types));
        return NullOpt{};
      }
    }

    if (tok.type == TokenType::identifier) {
      identifier_components.push_back(string_registry->register_string(tok.lexeme));
    }

    iterator.advance();
    expect_identifier = !expect_identifier;
  }

  const auto minimum_size = !is_matlab_import ? 1 : import_type == ImportType::wildcard ? 1 : 2;
  const int64_t num_components = identifier_components.size();

  if (num_components < minimum_size) {
    //  import; or import a;
    add_error(make_error_incomplete_import_stmt(parse_instance, source_token));
    return NullOpt{};
  }

  Import import(source_token, import_type, std::move(identifier_components));
  return Optional<Import>(std::move(import));
}

Optional<BoxedStmt> AstGenerator::command_stmt(const Token& source_token) {
  auto ident_res = one_identifier();
  if (!ident_res) {
    return NullOpt{};
  }

  bool should_proceed = true;
  std::vector<CharLiteralExpr> arguments;

  while (iterator.has_next() && should_proceed) {
    const auto& tok = iterator.peek();

    switch (tok.type) {
      case TokenType::null:
      case TokenType::new_line:
      case TokenType::semicolon:
      case TokenType::comma:
        should_proceed = false;
        break;
      default: {
        arguments.emplace_back(CharLiteralExpr(tok));
        iterator.advance();
      }
    }
  }

  std::array<TokenType, 4> possible_types{{
    TokenType::null, TokenType::new_line, TokenType::semicolon, TokenType::comma
  }};

  auto err = consume_one_of(possible_types.data(), possible_types.size());
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  int64_t command_identifier = string_registry->register_string(ident_res.value());

  auto node = std::make_unique<CommandStmt>(source_token, command_identifier, std::move(arguments));
  return Optional<BoxedStmt>(std::move(node));
}

Optional<BoxedStmt> AstGenerator::expr_stmt(const Token& source_token) {
  auto expr_res = expr();

  if (!expr_res) {
    return NullOpt{};
  }

  const auto& curr_token = iterator.peek();

  if (curr_token.type == TokenType::equal) {
    return assignment_stmt(expr_res.rvalue(), source_token);
  }

  auto expr_stmt = std::make_unique<ExprStmt>(expr_res.rvalue());
  return Optional<BoxedStmt>(std::move(expr_stmt));
}

Optional<BoxedStmt> AstGenerator::try_stmt(const mt::Token& source_token) {
  iterator.advance();

  //  Register an increase in the try statement scope depth.
  BlockStmtScopeHelper scope_helper(&block_depths.try_stmt);

  auto try_block_res = sub_block();
  if (!try_block_res) {
    return NullOpt{};
  }

  Optional<CatchBlock> catch_block = NullOpt{};
  const auto& maybe_catch_token = iterator.peek();

  if (maybe_catch_token.type == TokenType::keyword_catch) {
    iterator.advance();

    const bool allow_empty = true;  //  @Note: Allow empty catch expression.
    auto catch_expr_res = expr(allow_empty);

    if (!catch_expr_res) {
      return NullOpt{};
    }

    auto catch_block_res = sub_block();
    if (!catch_block_res) {
      return NullOpt{};
    }

    catch_block = CatchBlock(maybe_catch_token, catch_expr_res.rvalue(), catch_block_res.rvalue());
  }

  auto end_err = consume(TokenType::keyword_end);
  if (end_err) {
    add_error(end_err.rvalue());
    return NullOpt{};
  }

  auto node = std::make_unique<TryStmt>(source_token,
    try_block_res.rvalue(), std::move(catch_block));

  return Optional<BoxedStmt>(std::move(node));
}

Optional<BoxedAstNode> AstGenerator::stmt_or_function_def() {
  const auto& token = iterator.peek();
  if (token.type == TokenType::keyword_function) {
    auto def_res = function_def();
    if (!def_res) {
      return NullOpt{};
    } else {
      return Optional<BoxedAstNode>(def_res.rvalue());
    }
  } else {
    auto stmt_res = stmt();
    if (!stmt_res) {
      return NullOpt{};
    } else {
      return Optional<BoxedAstNode>(stmt_res.rvalue());
    }
  }
}

Optional<BoxedStmt> AstGenerator::stmt() {
  const auto& token = iterator.peek();

  switch (token.type) {
    case TokenType::keyword_return:
    case TokenType::keyword_break:
    case TokenType::keyword_continue:
      return control_stmt(token);

    case TokenType::keyword_for:
    case TokenType::keyword_parfor:
      return for_stmt(token);

    case TokenType::keyword_if:
      return if_stmt(token);

    case TokenType::keyword_switch:
      return switch_stmt(token);

    case TokenType::keyword_while:
      return while_stmt(token);

    case TokenType::keyword_try:
      return try_stmt(token);

    case TokenType::keyword_persistent:
    case TokenType::keyword_global:
      return variable_declaration_stmt(token);

    case TokenType::keyword_import:
      return import_stmt(token);

    default: {
      if (is_command_stmt(token)) {
        return command_stmt(token);
      } else {
        return expr_stmt(token);
      }
    }
  }
}

Optional<BoxedTypeAnnot> AstGenerator::type_annotation_macro(const Token& source_token) {
  iterator.advance();

  auto annot_res = type_annotation(iterator.peek());
  if (!annot_res) {
    return NullOpt{};
  }

  auto node = std::make_unique<TypeAnnotMacro>(source_token, annot_res.rvalue());
  return Optional<BoxedTypeAnnot>(std::move(node));
}

Optional<BoxedTypeAnnot> AstGenerator::inline_type_annotation(const mt::Token& source_token) {
  auto type_res = type(source_token);
  if (!type_res) {
    return NullOpt{};
  }

  auto err = consume(TokenType::keyword_end_type);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  auto node = std::make_unique<InlineType>(type_res.rvalue());
  return Optional<BoxedTypeAnnot>(std::move(node));
}

Optional<BoxedTypeAnnot> AstGenerator::type_annotation(const Token& source_token, bool expect_enclosing_assert_node) {
  switch (source_token.type) {
    case TokenType::keyword_begin:
      return type_begin(source_token);

    case TokenType::keyword_given:
      return type_scheme_declaration(source_token);

    case TokenType::keyword_let:
      return type_let(source_token);

    case TokenType::keyword_fun_type:
      return type_fun(source_token);

    case TokenType::double_colon:
      return type_assertion(source_token, expect_enclosing_assert_node, /*allow_scheme=*/ true);

    case TokenType::keyword_import:
      return type_import(source_token);

    case TokenType::keyword_record:
      return record_type_annot(source_token);

    case TokenType::keyword_declare:
      return declare_type(source_token);

    case TokenType::keyword_namespace:
      return type_namespace(source_token);

    case TokenType::keyword_constructor:
      return constructor_type(source_token);

    default:
      auto possible_types = type_annotation_block_possible_types();
      add_error(make_error_expected_token_type(parse_instance, source_token, possible_types));
      return NullOpt{};
  }
}

Optional<BoxedTypeAnnot> AstGenerator::type_assertion(const Token& source_token,
                                                      bool expect_enclosing_node,
                                                      bool allow_scheme) {
  iterator.advance();

  auto type_res = type(iterator.peek(), allow_scheme);
  if (!type_res) {
    return NullOpt{};
  }

  std::array<TokenType, 2> possible_types{{TokenType::keyword_end_type, TokenType::new_line}};
  auto err = consume_one_of(possible_types.data(), possible_types.size());
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  BoxedAstNode enclosing_node;

  if (expect_enclosing_node) {
    auto enclosing_res = stmt_or_function_def();
    if (!enclosing_res) {
      return NullOpt{};
    }
    enclosing_node = std::move(enclosing_res.rvalue());
  }

  //  Actual type not yet known.
  Type* resolved_type = nullptr;
  auto assert_node = std::make_unique<TypeAssertion>(source_token, std::move(enclosing_node),
                                                     type_res.rvalue(), resolved_type);
  return Optional<BoxedTypeAnnot>(std::move(assert_node));
}

Optional<BoxedTypeAnnot> AstGenerator::type_fun_enclosing_anonymous_function(const Token& source_token) {
  auto stmt_res = stmt();
  if (!stmt_res) {
    return NullOpt{};
  }

  auto stmt_node = std::move(stmt_res.rvalue());
  if (!stmt_node->is_assignment_stmt()) {
    add_error(make_error_non_assignment_stmt_in_fun_declaration(parse_instance, source_token));
    return NullOpt{};
  }

  const auto& assign_stmt = dynamic_cast<const AssignmentStmt&>(*stmt_node);
  if (!assign_stmt.of_expr->is_anonymous_function_expr()) {
    add_error(make_error_non_anonymous_function_rhs_in_fun_declaration(parse_instance, source_token));
    return NullOpt{};

  } else if (!assign_stmt.to_expr->is_identifier_reference_expr()) {
    add_error(make_error_non_identifier_lhs_in_fun_declaration(parse_instance, source_token));
    return NullOpt{};
  }

  const auto& ref_expr_lhs = dynamic_cast<const IdentifierReferenceExpr&>(*assign_stmt.to_expr);
  if (!ref_expr_lhs.subscripts.empty()) {
    add_error(make_error_non_identifier_lhs_in_fun_declaration(parse_instance, source_token));
    return NullOpt{};
  }

  auto node = std::make_unique<FunTypeNode>(source_token, std::move(stmt_node));
  return Optional<BoxedTypeAnnot>(std::move(node));
}

Optional<BoxedTypeAnnot> AstGenerator::type_fun_enclosing_function(const Token& source_token) {
  auto func_res = function_def();
  if (!func_res) {
    return NullOpt{};
  }

  auto node = std::make_unique<FunTypeNode>(source_token, std::move(func_res.rvalue()));
  return Optional<BoxedTypeAnnot>(std::move(node));
}

Optional<BoxedTypeAnnot> AstGenerator::type_fun(const mt::Token& source_token) {
  iterator.advance();

  auto end_type_err = consume(TokenType::keyword_end_type);
  if (end_type_err) {
    add_error(end_type_err.rvalue());
    return NullOpt{};
  }

  if (iterator.peek().type == TokenType::keyword_function) {
    return type_fun_enclosing_function(source_token);
  } else {
    return type_fun_enclosing_anonymous_function(source_token);
  }
}

Optional<BoxedTypeAnnot> AstGenerator::type_let(const mt::Token& source_token) {
  iterator.advance();
  const auto name_token = iterator.peek();

  auto ident_res = one_identifier();
  if (!ident_res) {
    return NullOpt{};
  }

  auto equal_err = consume(TokenType::equal);
  if (equal_err) {
    add_error(equal_err.rvalue());
    return NullOpt{};
  }

  auto equal_to_type_res = type(iterator.peek());
  if (!equal_to_type_res) {
    return NullOpt{};
  }

  auto identifier = TypeIdentifier(string_registry->register_string(ident_res.value()));
  identifier = maybe_namespace_enclose_type_identifier(identifier);

  auto scope = current_type_scope();
  const bool is_export = type_identifier_export_state.is_export();

  if (!scope->can_register_local_identifier(identifier, is_export)) {
    add_error(make_error_duplicate_type_identifier(parse_instance, name_token));
    return NullOpt{};
  }

  auto let_node = std::make_unique<TypeLet>(source_token, identifier, equal_to_type_res.rvalue());
  //  @Note: Source token must be a pointer to a source token stored in the AST.
  auto let_type = parse_instance->type_store->make_type_reference(&let_node->source_token, nullptr, scope);
  scope->emplace_type(identifier, let_type, is_export);

  return Optional<BoxedTypeAnnot>(std::move(let_node));
}

namespace {
  BoxedType make_scheme_type_node(AstGenerator* ast_gen,
                                  types::Scheme* scheme,
                                  BoxedType source_type,
                                  const Token& source_token,
                                  const std::vector<std::string_view>& str_identifiers) {
    auto identifiers = ast_gen->string_registry->register_strings(str_identifiers);

    auto& type_store = ast_gen->parse_instance->type_store;
    for (const auto& ident : identifiers) {
      (void) ident;
      scheme->parameters.push_back(type_store->make_fresh_type_variable_reference());
    }

    return std::make_unique<SchemeTypeNode>(source_token, std::move(identifiers),
                                            std::move(source_type), scheme);
  }
}

Optional<BoxedTypeAnnot> AstGenerator::type_scheme_declaration(const Token& source_token) {
  iterator.advance();

  const auto& ident_token = iterator.peek();
  auto identifier_res = type_variable_identifiers(ident_token);
  if (!identifier_res) {
    return NullOpt{};
  }

  if (identifier_res.value().empty()) {
    add_error(make_error_expected_non_empty_type_variable_identifiers(parse_instance, ident_token));
    return NullOpt{};
  }

  auto new_enclosing_scheme = parse_instance->type_store->make_scheme();
  enclosing_schemes.push_back(new_enclosing_scheme);
  MT_SCOPE_EXIT {
    enclosing_schemes.pop_back();
  };

  const auto& decl_token = iterator.peek();
  Optional<BoxedType> source_type_res;

  switch (decl_token.type) {
    case TokenType::keyword_record:
      source_type_res = type_record(decl_token);
      break;

    default: {
      std::array<TokenType, 1> possible_types{{TokenType::keyword_record}};
      add_error(make_error_expected_token_type(parse_instance, decl_token, possible_types));
      return NullOpt{};
    }
  }

  if (!source_type_res) {
    return NullOpt{};
  }

  auto scheme_node = make_scheme_type_node(this, new_enclosing_scheme,
    std::move(source_type_res.rvalue()), source_token, identifier_res.value());

  return Optional<BoxedTypeAnnot>(std::move(scheme_node));
}

Optional<std::vector<std::string_view>> AstGenerator::type_variable_identifiers(const mt::Token& source_token) {
  std::vector<std::string_view> type_variable_identifiers;
  std::unordered_set<std::string_view> unique_identifiers;

  if (source_token.type == TokenType::less) {
    iterator.advance();
    auto type_variable_res = identifier_sequence(TokenType::greater);
    if (!type_variable_res) {
      return NullOpt{};
    }
    type_variable_identifiers = type_variable_res.rvalue();

    for (const auto& ident : type_variable_identifiers) {
      auto curr_size = unique_identifiers.size();
      unique_identifiers.insert(ident);
      if (unique_identifiers.size() != curr_size + 1) {
        add_error(make_error_duplicate_type_identifier(parse_instance, source_token));
        return NullOpt{};
      }
    }

  } else if (source_token.type == TokenType::identifier) {
    iterator.advance();
    type_variable_identifiers.emplace_back(source_token.lexeme);

  } else {
    std::array<TokenType, 2> possible_types{{TokenType::less, TokenType::identifier}};
    add_error(make_error_expected_token_type(parse_instance, source_token, possible_types));
    return NullOpt{};
  }

  return Optional<std::vector<std::string_view>>(std::move(type_variable_identifiers));
}

Optional<std::vector<BoxedTypeAnnot>> AstGenerator::type_annotation_block() {
  std::vector<BoxedTypeAnnot> contents;

  while (iterator.has_next() && iterator.peek().type != TokenType::keyword_end_type) {
    const auto& token = iterator.peek();

    switch (token.type) {
      case TokenType::new_line:
      case TokenType::null:
        iterator.advance();
        break;
      default: {
        auto annot_res = type_annotation(token);
        if (annot_res) {
          contents.emplace_back(annot_res.rvalue());
        } else {
          const auto possible_types = type_annotation_block_possible_types();
          iterator.advance_to_one(possible_types.data(), possible_types.size());
          return NullOpt{};
        }
      }
    }
  }

  return Optional<std::vector<BoxedTypeAnnot>>(std::move(contents));
}

Optional<BoxedTypeAnnot> AstGenerator::type_begin(const mt::Token& source_token) {
  iterator.advance();

  const bool is_exported = iterator.peek().type == TokenType::keyword_export;
  if (is_exported) {
    iterator.advance();
  }

  //  Mark whether the contained identifiers are exported.
  TypeIdentifierExportState::Helper export_state_helper(type_identifier_export_state, is_exported);

  auto contents_res = type_annotation_block();
  if (!contents_res) {
    return NullOpt{};
  }

  auto err = consume(TokenType::keyword_end_type);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  auto node = std::make_unique<TypeBegin>(source_token, is_exported, std::move(contents_res.rvalue()));
  return Optional<BoxedTypeAnnot>(std::move(node));
}

Optional<BoxedTypeAnnot> AstGenerator::type_import(const Token& source_token) {
  iterator.advance();

  auto component_res = compound_identifier_components();
  if (!component_res) {
    return NullOpt{};
  }

  auto registered = string_registry->register_strings(component_res.value());
  auto joined_ident = string_registry->make_registered_compound_identifier(registered);

  auto import_node = std::make_unique<TypeImportNode>(source_token, joined_ident);
  const bool is_export = type_identifier_export_state.is_export();

  PendingTypeImport pending_type_import(source_token, joined_ident, current_type_scope(), is_export);
  parse_instance->pending_type_imports.push_back(pending_type_import);

  //  Allows @T import X
  consume_if_matches(TokenType::keyword_end_type);

  return Optional<BoxedTypeAnnot>(std::move(import_node));
}

Optional<BoxedTypeAnnot> AstGenerator::type_namespace(const Token& source_token) {
  iterator.advance();

  auto ident_res = one_identifier();
  if (!ident_res) {
    return NullOpt{};
  }

  TypeIdentifier namespace_identifier(string_registry->register_string(ident_res.value()));
  TypeIdentifierNamespaceState::Helper namespace_helper(namespace_state, namespace_identifier);

  auto contents_res = type_annotation_block();
  if (!contents_res) {
    return NullOpt{};
  }

  auto err = consume(TokenType::keyword_end_type);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  auto node = std::make_unique<NamespaceTypeNode>(source_token, namespace_identifier, std::move(contents_res.rvalue()));
  return Optional<BoxedTypeAnnot>(std::move(node));
}

Optional<BoxedTypeAnnot> AstGenerator::scalar_type_declaration(const Token& source_token) {
  const auto ident_token = iterator.peek();

  auto ident_res = one_identifier();
  if (!ident_res) {
    return NullOpt{};
  }

  auto ident = TypeIdentifier(string_registry->register_string(ident_res.value()));
  ident = maybe_namespace_enclose_type_identifier(ident);

  auto scope = current_type_scope();
  const bool is_export = type_identifier_export_state.is_export();

  if (!scope->can_register_local_identifier(ident, is_export)) {
    add_error(make_error_duplicate_type_identifier(parse_instance, ident_token));
    return NullOpt{};
  }

  auto type = parse_instance->library->make_named_scalar_type(ident);
  auto node = std::make_unique<DeclareTypeNode>(source_token, DeclareTypeNode::Kind::scalar, ident);
  auto declare_type = parse_instance->type_store->make_type_reference(&node->source_token, type, scope);
  scope->emplace_type(ident, declare_type, is_export);

  return Optional<BoxedTypeAnnot>(std::move(node));
}

namespace {
  Optional<BoxedType> underlying_type_of_assertion(AstGenerator* ast_gen) {
    const auto assert_tok = ast_gen->iterator.peek();
    auto annot_res =
      ast_gen->type_annotation(ast_gen->iterator.peek(), /*expect_enclosing_assert_node=*/false);
    if (!annot_res) {
      return NullOpt{};
    }

    auto& annot = annot_res.value();
    if (!annot->is_type_assertion()) {
      auto err = make_error_expected_type_assertion(ast_gen->parse_instance, assert_tok);
      ast_gen->add_error(std::move(err));
      return NullOpt{};
    }

    auto type_assert = static_cast<TypeAssertion*>(annot.get());
    return Optional<BoxedType>(std::move(type_assert->has_type));
  }
}

Optional<BoxedTypeAnnot> AstGenerator::function_type_declaration(const Token& source_token) {
  iterator.advance();

  auto ident_res = one_identifier();
  if (!ident_res) {
    return NullOpt{};
  }

  auto type_assert_res = underlying_type_of_assertion(this);
  if (!type_assert_res) {
    return NullOpt{};
  }

  auto underlying = std::move(type_assert_res.rvalue());
  if (!underlying->is_function_type()) {
    add_error(make_error_expected_function_type(parse_instance, source_token));
    return NullOpt{};
  }

  auto func_type_ptr = static_cast<FunctionTypeNode*>(underlying.release());
  std::unique_ptr<FunctionTypeNode> func_type_node(func_type_ptr);

  TypeIdentifier ident(string_registry->register_string(ident_res.value()));
  ident = maybe_namespace_enclose_type_identifier(ident);

  if (parse_instance->library->has_declared_function_type(ident)) {
    add_error(make_error_duplicate_local_function(parse_instance, source_token));
    return NullOpt{};
  } else {
    parse_instance->library->register_declared_function(ident);
  }

  auto declare_node =
    std::make_unique<DeclareFunctionTypeNode>(source_token, ident, std::move(func_type_node));

  return Optional<BoxedTypeAnnot>(std::move(declare_node));
}

Optional<BoxedTypeAnnot> AstGenerator::method_type_declaration(const Token& source_token) {
  auto type_res = compound_identifier_components();
  if (!type_res) {
    return NullOpt{};
  }

  auto registered = string_registry->register_strings(type_res.value());
  auto type_ident = TypeIdentifier(string_registry->make_registered_compound_identifier(registered));
  type_ident = maybe_namespace_enclose_type_identifier(type_ident);

  auto func_token = iterator.peek();
  iterator.advance();

  auto type_assert_res = underlying_type_of_assertion(this);
  if (!type_assert_res) {
    return NullOpt{};
  }

  auto underlying = std::move(type_assert_res.rvalue());
  if (!underlying->is_function_type()) {
    add_error(make_error_expected_function_type(parse_instance, func_token));
    return NullOpt{};
  }

  auto underlying_ptr = static_cast<FunctionTypeNode*>(underlying.release());
  auto func_type = std::unique_ptr<FunctionTypeNode>(underlying_ptr);

  DeclareTypeNode::Method method;

  if (represents_binary_operator(func_token.type)) {
    method = DeclareTypeNode::Method(binary_operator_from_token_type(func_token.type), std::move(func_type));

  } else if (represents_unary_operator(func_token.type)) {
    method = DeclareTypeNode::Method(unary_operator_from_token_type(func_token.type), std::move(func_type));

  } else if (func_token.type == TokenType::identifier) {
    TypeIdentifier name(string_registry->register_string(func_token.lexeme));
    method = DeclareTypeNode::Method(name, std::move(func_type));

  } else {
    add_error(make_error_expected_function_type(parse_instance, func_token));
    return NullOpt{};
  }

  auto node = std::make_unique<DeclareTypeNode>(source_token, type_ident, std::move(method));
  return Optional<BoxedTypeAnnot>(std::move(node));
}

Optional<BoxedTypeAnnot> AstGenerator::declare_type(const Token& source_token) {
  iterator.advance();

  const auto kind_token = iterator.peek();
  if (kind_token.type == TokenType::keyword_function_type) {
    return function_type_declaration(kind_token);
  }

  auto kind_res = one_identifier();
  if (!kind_res) {
    return NullOpt{};
  }

  auto maybe_kind = DeclareTypeNode::kind_from_string(kind_res.value());
  if (!maybe_kind) {
    add_error(make_error_unrecognized_type_declaration_kind(parse_instance, kind_token));
    return NullOpt{};
  }

  switch (maybe_kind.value()) {
    case DeclareTypeNode::Kind::scalar:
      return scalar_type_declaration(source_token);

    case DeclareTypeNode::Kind::method:
      return method_type_declaration(source_token);

    default:
      assert(false);
      return Optional<BoxedTypeAnnot>(nullptr);
  }
}

Optional<BoxedTypeAnnot> AstGenerator::record_type_annot(const Token& source_token) {
  auto res = type_record(source_token);
  if (!res) {
    return NullOpt{};
  }

  return Optional<BoxedTypeAnnot>(upcast<TypeNode, TypeAnnot>(res.rvalue()));
}

Optional<BoxedTypeAnnot> AstGenerator::constructor_type(const Token& source_token) {
  iterator.advance();

  auto maybe_err = consume(TokenType::keyword_end_type);
  if (maybe_err) {
    add_error(maybe_err.rvalue());
    return NullOpt{};
  }

  auto stmt_res = expr_stmt(iterator.peek());
  if (!stmt_res) {
    return NullOpt{};
  } else if (!stmt_res.value()->is_assignment_stmt()) {
    add_error(make_error_expected_assignment_stmt_in_ctor(parse_instance, source_token));
    return NullOpt{};
  }

  auto stmt_ptr = static_cast<AssignmentStmt*>(stmt_res.value().release());
  auto stmt = std::unique_ptr<AssignmentStmt>(stmt_ptr);

  if (!stmt->of_expr->is_identifier_reference_expr()) {
    add_error(make_error_expected_identifier_reference_expr_in_ctor(parse_instance, source_token));
    return NullOpt{};
  }

  auto expr_ptr = static_cast<IdentifierReferenceExpr*>(stmt->of_expr.get());
  auto full_name = expr_ptr->primary_identifier.full_name();

  if (full_name != parse_instance->library->special_identifiers.identifier_struct ||
      !expr_ptr->is_maybe_non_subscripted_function_call()) {
    //  Ensure identifier reference expression is of the form struct(a, b, c);
    add_error(make_error_expected_struct_function_call(parse_instance, source_token));
    return NullOpt{};

  } else if ((expr_ptr->num_primary_subscript_arguments() % 2) != 0) {
    add_error(make_error_expected_even_number_of_arguments_in_ctor(parse_instance, source_token));
    return NullOpt{};
  }

  auto node = std::make_unique<ConstructorTypeNode>(source_token, std::move(stmt), expr_ptr);
  return Optional<BoxedTypeAnnot>(std::move(node));
}

namespace {

bool maybe_record_field(AstGenerator* instance, RecordTypeNode::Fields& fields,
                        RecordTypeNode::FieldNames& field_names) {
  auto tok = instance->iterator.peek();
  if (tok.type == TokenType::new_line) {
    instance->iterator.advance();
    return true;
  }

  auto field_res = instance->record_field();
  if (!field_res) {
    return false;
  }

  auto& field = field_res.value();
  if (field_names.count(field.name) > 0) {
    instance->add_error(make_error_duplicate_record_field_name(instance->parse_instance, tok));
    return false;
  }

  field_names.insert(field.name);
  fields.push_back(std::move(field));

  return true;
}

}

Optional<BoxedType> AstGenerator::type_scheme(const Token& source_token) {
  iterator.advance();

  auto scheme_idents = type_variable_identifiers(iterator.peek());
  if (!scheme_idents) {
    return NullOpt{};
  } else if (scheme_idents.value().empty()) {
    add_error(make_error_expected_non_empty_type_variable_identifiers(parse_instance, source_token));
    return NullOpt{};
  }

  auto source_type_res = type(iterator.peek());
  if (!source_type_res) {
    return NullOpt{};
  }

  auto scheme = parse_instance->type_store->make_scheme();
  auto scheme_type_node = make_scheme_type_node(this, scheme, std::move(source_type_res.rvalue()),
    source_token, scheme_idents.value());

  return Optional<BoxedType>(std::move(scheme_type_node));
}

Optional<BoxedType> AstGenerator::type_record(const Token& source_token) {
  iterator.advance();
  const auto name_token = iterator.peek();

  auto ident_res = one_identifier();
  if (!ident_res) {
    return NullOpt{};
  }

  auto maybe_enclosing_scheme = enclosing_schemes.back();

  //  No longer in a scheme.
  enclosing_schemes.push_back(nullptr);
  MT_SCOPE_EXIT {
    enclosing_schemes.pop_back();
  };

  RecordTypeNode::Fields fields;
  RecordTypeNode::FieldNames field_names;

  while (iterator.has_next() && iterator.peek().type != TokenType::keyword_end_type) {
    bool success = maybe_record_field(this, fields, field_names);
    if (!success) {
      return NullOpt{};
    }
  }

  auto err = consume(TokenType::keyword_end_type);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  auto identifier = TypeIdentifier(string_registry->register_string(ident_res.value()));
  identifier = maybe_namespace_enclose_type_identifier(identifier);

  auto scope = current_type_scope();
  const bool is_export = type_identifier_export_state.is_export();

  if (!scope->can_register_local_identifier(identifier, is_export)) {
    add_error(make_error_duplicate_type_identifier(parse_instance, name_token));
    return NullOpt{};
  }

  auto& type_store = parse_instance->type_store;
  auto record_type = type_store->make_record();
  auto record_node = std::make_unique<RecordTypeNode>(source_token, name_token, identifier,
                                                      record_type, std::move(fields));
  Type* emplaced_type = record_type;

  if (maybe_enclosing_scheme) {
    assert(!maybe_enclosing_scheme->type);
//    maybe_enclosing_scheme->type = record_type;
    emplaced_type = maybe_enclosing_scheme;
  }

  //  @Note: Source token must be a pointer to a source token stored in the AST.
  const auto type_ref = type_store->make_type_reference(&record_node->name_token, emplaced_type, scope);
  scope->emplace_type(identifier, type_ref, is_export);

  return Optional<BoxedType>(std::move(record_node));
}

TypeIdentifier AstGenerator::maybe_namespace_enclose_type_identifier(const TypeIdentifier& ident) const {
  if (namespace_state.has_enclosing_namespace()) {
    std::vector<int64_t> namespace_components;
    namespace_state.gather_components(namespace_components);
    namespace_components.push_back(ident.full_name());
    return TypeIdentifier(string_registry->make_registered_compound_identifier(namespace_components));
  } else {
    return ident;
  }
}

Optional<RecordTypeNode::Field> AstGenerator::record_field() {
  const auto name_token = iterator.peek();
  auto name_res = one_identifier();
  if (!name_res) {
    return NullOpt{};
  }

  auto err = consume(TokenType::colon);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  auto type_res = type(iterator.peek());
  if (!type_res) {
    return NullOpt{};
  }

  auto name = TypeIdentifier(string_registry->register_string(name_res.value()));
  return Optional<RecordTypeNode::Field>(RecordTypeNode::Field(name_token, name, std::move(type_res.rvalue())));
}

Optional<BoxedType> AstGenerator::type(const mt::Token& source_token, bool allow_scheme) {
  if (allow_scheme && source_token.type == TokenType::keyword_given) {
    return type_scheme(source_token);
  }

  auto first_type_res = one_type(source_token);
  if (!first_type_res) {
    return NullOpt{};
  }

  if (iterator.peek().type != TokenType::vertical_bar) {
    return first_type_res;
  }

  std::vector<BoxedType> union_type_members;
  union_type_members.emplace_back(first_type_res.rvalue());

  while (iterator.has_next() && iterator.peek().type == TokenType::vertical_bar) {
    iterator.advance();

    auto next_type_res = one_type(iterator.peek());
    if (!next_type_res) {
      return NullOpt{};
    }

    union_type_members.emplace_back(next_type_res.rvalue());
  }

  auto union_type_node = std::make_unique<UnionTypeNode>(std::move(union_type_members));
  return Optional<BoxedType>(std::move(union_type_node));
}

Optional<BoxedType> AstGenerator::one_type(const mt::Token& source_token) {
  switch (source_token.type) {
    case TokenType::left_bracket:
      return function_type(source_token);

    case TokenType::left_brace:
      return tuple_type(source_token);

    case TokenType::identifier:
      return scalar_type(source_token);

    case TokenType::question:
      return infer_type(source_token);

    case TokenType::keyword_list:
      return list_type(source_token);

    default: {
      std::array<TokenType, 5> possible_types{{TokenType::left_bracket, TokenType::identifier,
                                               TokenType::question, TokenType::left_brace,
                                               TokenType::keyword_list}};
      add_error(make_error_expected_token_type(parse_instance, source_token, possible_types));
      return NullOpt{};
    }
  }
}

Optional<std::vector<BoxedType>> AstGenerator::type_sequence(TokenType terminator) {
  std::vector<BoxedType> types;

  while (iterator.has_next() && iterator.peek().type != terminator) {
    const auto& source_token = iterator.peek();

    auto type_res = type(source_token);
    if (!type_res) {
      return NullOpt{};
    }

    types.emplace_back(type_res.rvalue());

    const auto next_type = iterator.peek().type;
    if (next_type == TokenType::comma) {
      iterator.advance();

    } else if (next_type != terminator) {
      std::array<TokenType, 2> possible_types{{TokenType::comma, terminator}};
      add_error(make_error_expected_token_type(parse_instance, iterator.peek(), possible_types));
      return NullOpt{};
    }
  }

  auto err = consume(terminator);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  return Optional<std::vector<BoxedType>>(std::move(types));
}

Optional<BoxedType> AstGenerator::infer_type(const Token& source_token) {
  iterator.advance();

  auto type = parse_instance->type_store->make_fresh_type_variable_reference();
  auto node = std::make_unique<InferTypeNode>(source_token, type);

  return Optional<BoxedType>(std::move(node));
}

Optional<BoxedType> AstGenerator::list_type(const Token& source_token) {
  iterator.advance();

  auto lt_err = consume(TokenType::less);
  if (lt_err) {
    add_error(lt_err.rvalue());
    return NullOpt{};
  }

  auto type_res = type_sequence(TokenType::greater);
  if (!type_res) {
    return NullOpt{};
  }

  auto node = std::make_unique<ListTypeNode>(source_token, std::move(type_res.rvalue()));
  return Optional<BoxedType>(std::move(node));
}

Optional<BoxedType> AstGenerator::function_type(const Token& source_token) {
  iterator.advance();

  auto output_res = type_sequence(TokenType::right_bracket);
  if (!output_res) {
    return NullOpt{};
  }

  auto equal_err = consume(TokenType::equal);
  if (equal_err) {
    add_error(equal_err.rvalue());
    return NullOpt{};
  }

  auto parens_err = consume(TokenType::left_parens);
  if (parens_err) {
    add_error(parens_err.rvalue());
    return NullOpt{};
  }

  auto input_res = type_sequence(TokenType::right_parens);
  if (!input_res) {
    return NullOpt{};
  }

  //  Resolved type not yet known.
  Type* resolved_type = nullptr;
  auto type_node = std::make_unique<FunctionTypeNode>(source_token, output_res.rvalue(),
                                                      input_res.rvalue(), resolved_type);
  return Optional<BoxedType>(std::move(type_node));
}

Optional<BoxedType> AstGenerator::scalar_type(const mt::Token& source_token) {
  auto ident_res = compound_identifier_components();
  if (!ident_res) {
    return NullOpt{};
  }
  auto registered = string_registry->register_strings(ident_res.value());
  TypeIdentifier identifier(string_registry->make_registered_compound_identifier(registered));

  std::vector<BoxedType> arguments;

  if (iterator.peek().type == TokenType::less) {
    iterator.advance();

    auto argument_res = type_sequence(TokenType::greater);
    if (!argument_res) {
      return NullOpt{};
    } else {
      arguments = argument_res.rvalue();
    }
  }

  auto node = std::make_unique<ScalarTypeNode>(source_token, identifier, std::move(arguments));
  return Optional<BoxedType>(std::move(node));
}

Optional<BoxedType> AstGenerator::tuple_type(const Token& source_token) {
  iterator.advance();

  auto seq_res = type_sequence(TokenType::right_brace);
  if (!seq_res) {
    return NullOpt{};
  }

  auto node = std::make_unique<TupleTypeNode>(source_token, std::move(seq_res.rvalue()));
  return Optional<BoxedType>(std::move(node));
}

bool AstGenerator::is_unary_prefix_expr(const mt::Token& curr_token) const {
  return represents_prefix_unary_operator(curr_token.type) &&
    (curr_token.type == TokenType::tilde ||
    can_precede_prefix_unary_operator(iterator.peek_prev().type));
}

bool AstGenerator::is_colon_subscript_expr(const mt::Token& curr_token) const {
  if (curr_token.type != TokenType::colon) {
    return false;
  }

  auto next_type = iterator.peek_next().type;
  return represents_grouping_terminator(next_type) || next_type == TokenType::comma;
}

bool AstGenerator::is_ignore_argument_expr(const mt::Token& curr_token) const {
  //  [~] = func() | [~, a] = func()
  if (curr_token.type != TokenType::tilde) {
    return false;
  }

  const auto& next = iterator.peek_nth(1);
  return next.type == TokenType::comma || next.type == TokenType::right_bracket;
}

bool AstGenerator::is_command_stmt(const mt::Token& curr_token) const {
  const auto next_t = iterator.peek_next().type;

  return curr_token.type == TokenType::identifier &&
    (represents_literal(next_t) || next_t == TokenType::identifier);
}

bool AstGenerator::is_within_end_terminated_stmt_block() const {
  return block_depths.try_stmt > 0 || block_depths.switch_stmt > 0 || block_depths.if_stmt > 0 ||
    is_within_loop();
}

bool AstGenerator::is_within_top_level_function() const {
  return block_depths.function_def == 1;
}

bool AstGenerator::is_within_class() const {
  return block_depths.class_def > 0;
}

bool AstGenerator::root_is_external_method() const {
  return parse_instance->treat_root_as_external_method;
}

bool AstGenerator::is_within_loop() const {
  return block_depths.for_stmt > 0 || block_depths.parfor_stmt > 0 || block_depths.while_stmt > 0;
}

void AstGenerator::push_scope() {
  push_scope(current_scope(), current_type_scope());
}

void AstGenerator::push_scope(MatlabScope* curr_matlab_scope, TypeScope* curr_type_scope) {
  const auto root_tscope = root_type_scope();

  Store::Write writer(*store);
  scopes.push_back(writer.make_matlab_scope(curr_matlab_scope, parse_instance->file_descriptor()));

  auto new_type_scope = writer.make_type_scope(root_tscope, curr_type_scope);
  type_scopes.push_back(new_type_scope);

  if (root_tscope == nullptr) {
    new_type_scope->root = new_type_scope;
  } else {
    curr_type_scope->add_child(new_type_scope);
  }
}

void AstGenerator::pop_scope() {
  assert(scopes.size() > 0 && type_scopes.size() > 0 && "No scope to pop.");
  scopes.pop_back();
  type_scopes.pop_back();
}

void AstGenerator::push_function_attributes(const FunctionAttributes& attrs) {
  function_attributes.push_back(attrs);
}

const FunctionAttributes& AstGenerator::current_function_attributes() const {
  assert(!function_attributes.empty() && "No function attributes.");
  return function_attributes.back();
}

void AstGenerator::pop_function_attributes() {
  assert(function_attributes.size() > 0 && "No attributes to pop.");
  function_attributes.pop_back();
}

MatlabScope* AstGenerator::current_scope() {
  return scopes.empty() ? nullptr : scopes.back();
}

MatlabScope* AstGenerator::root_scope() {
  return scopes.empty() ? nullptr : scopes.front();
}

TypeScope* AstGenerator::current_type_scope() {
  return type_scopes.empty() ? nullptr : type_scopes.back();
}

TypeScope* AstGenerator::root_type_scope() {
  return type_scopes.empty() ? nullptr : type_scopes.front();
}

void AstGenerator::register_import(mt::Import&& import) {
  current_scope()->register_import(std::move(import));
}

Optional<ParseError> AstGenerator::consume(mt::TokenType type) {
  const auto& tok = iterator.peek();

  if (tok.type == type) {
    iterator.advance();
    return NullOpt{};
  } else {
    return Optional<ParseError>(make_error_expected_token_type(parse_instance, tok, &type, 1));
  }
}

Optional<ParseError> AstGenerator::consume_one_of(const mt::TokenType* types, int64_t num_types) {
  const auto& tok = iterator.peek();

  for (int64_t i = 0; i < num_types; i++) {
    if (types[i] == tok.type) {
      iterator.advance();
      return NullOpt{};
    }
  }

  return Optional<ParseError>(make_error_expected_token_type(parse_instance, tok, types, num_types));
}

void AstGenerator::consume_if_matches(TokenType type) {
  if (iterator.peek().type == type) {
    iterator.advance();
  }
}

Optional<ParseError>
AstGenerator::check_parameters_are_unique(const Token& source_token,
                                          const FunctionParameters& params) const {
  std::set<int64_t> uniques;
  for (const auto& input : params) {
    if (input.is_ignored()) {
      //  Input argument is ignored (~)
      continue;
    }

    auto orig_size = uniques.size();
    uniques.insert(input.name.full_name());

    if (orig_size == uniques.size()) {
      //  This value is not new.
      return Optional<ParseError>(make_error_duplicate_input_parameter_in_expr(parse_instance, source_token));
    }
  }
  return NullOpt{};
}

void AstGenerator::add_error(mt::ParseError&& err) {
  parse_instance->errors.emplace_back(std::move(err));
}

}
