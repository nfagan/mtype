#include "ast_gen.hpp"
#include "string.hpp"
#include "utility.hpp"
#include "keyword.hpp"
#include <array>
#include <cassert>
#include <functional>
#include <set>

namespace mt {

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

struct ParseScopeHelper : public ScopeHelper<AstGenerator, ParseScopeStack> {
  using ScopeHelper::ScopeHelper;
};

struct FunctionAttributeStack {
  static void push(AstGenerator&) {
    //
  }
  static void pop(AstGenerator& gen) {
    gen.pop_function_attributes();
  }
};

struct FunctionAttributeHelper : public ScopeHelper<AstGenerator, FunctionAttributeStack> {
  using ScopeHelper::ScopeHelper;
};

Result<ParseErrors, BoxedRootBlock>
AstGenerator::parse(const std::vector<Token>& tokens, std::string_view txt, const ParseInputs& inputs) {
  iterator = TokenIterator(&tokens);
  text = txt;
  is_end_terminated_function = inputs.functions_are_end_terminated;
  string_registry = inputs.string_registry;
  store = inputs.store;

  scope_handles.clear();
  function_attributes.clear();
  parse_errors.clear();

  block_depths = BlockDepths();
  class_state = ClassDefState();

  //  Push root scope.
  ParseScopeHelper scope_helper(*this);

  //  Push default function attributes.
  push_function_attributes(FunctionAttributes());
  FunctionAttributeHelper attr_helper(*this);

  //  Main block.
  auto result = block();

  if (result) {
    auto block_node = std::move(result.rvalue());
    auto scope_handle = current_scope_handle();

    auto root_node = std::make_unique<RootBlock>(std::move(block_node), scope_handle);
    return make_success<ParseErrors, BoxedRootBlock>(std::move(root_node));
  } else {
    return make_error<ParseErrors, BoxedRootBlock>(std::move(parse_errors));
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

Optional<std::vector<FunctionInputParameter>> AstGenerator::anonymous_function_input_parameters() {
  std::vector<FunctionInputParameter> input_parameters;
  bool expect_parameter = true;

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
      input_parameters.emplace_back(string_registry->register_string(tok.lexeme));

    } else if (tok.type == TokenType::tilde) {
      //  @(~, y)
      input_parameters.emplace_back();
    }

    expect_parameter = !expect_parameter;
  }

  auto err = consume(TokenType::right_parens);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  return Optional<std::vector<FunctionInputParameter>>(std::move(input_parameters));
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
        add_error(make_error_expected_token_type(tok, &expected_type, 1));
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

Optional<FunctionHeader> AstGenerator::function_header() {
  bool provided_outputs;
  auto output_res = function_outputs(&provided_outputs);
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
      add_error(make_error_invalid_period_qualified_function_def(iterator.peek()));
      return NullOpt{};
    }
  }

  auto input_res = function_inputs();
  if (!input_res) {
    return NullOpt{};
  }

  if (!use_compound_name) {
    auto single_name_component = string_registry->register_string(name_res.value());
    name = MatlabIdentifier(single_name_component);
  }

  auto outputs = string_registry->register_strings(output_res.value());

  FunctionHeader header(name_token, name, std::move(outputs), std::move(input_res.rvalue()));
  return Optional<FunctionHeader>(std::move(header));
}

Optional<std::vector<FunctionInputParameter>> AstGenerator::function_inputs() {
  if (iterator.peek().type == TokenType::left_parens) {
    iterator.advance();
    return anonymous_function_input_parameters();
  } else {
    //  Function declarations without parentheses are valid and indicate 0 inputs.
    return Optional<std::vector<FunctionInputParameter>>(std::vector<FunctionInputParameter>());
  }
}

Optional<std::vector<std::string_view>> AstGenerator::function_outputs(bool* provided_outputs) {
  const auto& tok = iterator.peek();
  std::vector<std::string_view> outputs;
  *provided_outputs = false;

  if (tok.type == TokenType::left_bracket) {
    //  function [a, b, c] = example()
    iterator.advance();
    *provided_outputs = true;
    return identifier_sequence(TokenType::right_bracket);

  } else if (tok.type == TokenType::identifier) {
    //  Either: function a = example() or function example(). I.e., the next identifier is either
    //  the single output parameter of the function or the name of the function.
    const auto& next_tok = iterator.peek_nth(1);
    const bool is_single_output = next_tok.type == TokenType::equal;

    if (is_single_output) {
      *provided_outputs = true;
      outputs.push_back(tok.lexeme);
      iterator.advance();
    }

    return Optional<std::vector<std::string_view>>(std::move(outputs));

  } else {
    std::array<TokenType, 2> possible_types{{TokenType::left_bracket, TokenType::identifier}};
    add_error(make_error_expected_token_type(tok, possible_types));
    return NullOpt{};
  }
}

Optional<std::unique_ptr<FunctionDefNode>> AstGenerator::function_def() {
  const auto& source_token = iterator.peek();
  iterator.advance();

  //  Enter function keyword
  BlockStmtScopeHelper block_depth_helper(&block_depths.function_def);

  if (is_within_end_terminated_stmt_block()) {
    //  It's illegal to define a function within an if statement, etc.
    add_error(make_error_invalid_function_def_location(source_token));
    return NullOpt{};
  }

  auto header_result = function_header();

  if (!header_result) {
    return NullOpt{};
  }

  Optional<std::unique_ptr<Block>> body_res;
  MatlabScopeHandle child_scope;

  {
    //  Increment scope, and decrement upon block exit.
    ParseScopeHelper scope_helper(*this);
    child_scope = current_scope_handle();

    body_res = sub_block();
    if (!body_res) {
      return NullOpt{};
    }

    if (is_end_terminated_function) {
      auto err = consume(TokenType::keyword_end);
      if (err) {
        add_error(err.rvalue());
        return NullOpt{};
      }
    }
  }

  const FunctionAttributes& attrs = current_function_attributes();
  FunctionDef function_def(header_result.rvalue(), attrs, body_res.rvalue());
  const auto name = function_def.header.name;

  //  Function registry owns the local function definition.
  FunctionDefHandle def_handle;
  FunctionReferenceHandle ref_handle;

  {
    Store::Write writer(*store);
    def_handle = writer.emplace_definition(std::move(function_def));
    ref_handle = writer.make_local_reference(name, def_handle, child_scope);
  }

  Store::ReadMut reader(*store);
  auto& parent_scope = reader.at(current_scope_handle());

  bool register_success = true;

  if (!is_within_class() || !is_within_top_level_function()) {
    //  In a classdef file, adjacent top-level methods are not visible to one another, so we don't
    //  register them in the parent's scope.
    register_success = parent_scope.register_local_function(name, ref_handle);
  }

  if (!register_success) {
    add_error(make_error_duplicate_local_function(source_token));
    return NullOpt{};
  }

  auto ast_node = std::make_unique<FunctionDefNode>(def_handle, child_scope);
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

Optional<std::vector<MatlabIdentifier>> AstGenerator::superclass_names() {
  std::vector<MatlabIdentifier> names;

  while (iterator.has_next()) {
    auto one_name_res = superclass_name();
    if (!one_name_res) {
      return NullOpt{};
    }

    names.emplace_back(one_name_res.rvalue());

    if (iterator.peek().type == TokenType::ampersand) {
      iterator.advance();
    } else {
      break;
    }
  }

  return Optional<std::vector<MatlabIdentifier>>(std::move(names));
}

Optional<BoxedAstNode> AstGenerator::class_def() {
  const auto& source_token = iterator.peek();
  iterator.advance();

  const auto parent_scope_handle = current_scope_handle();
  BlockStmtScopeHelper scope_helper(&block_depths.class_def);

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

  auto name_res = one_identifier();
  if (!name_res) {
    return NullOpt{};
  }

  std::vector<MatlabIdentifier> superclasses;

  //  classdef X < Y
  if (iterator.peek().type == TokenType::less) {
    iterator.advance();

    auto superclass_res = superclass_names();
    if (!superclass_res) {
      return NullOpt{};
    }

    superclasses = std::move(superclass_res.rvalue());
  }

  ClassDef::Properties properties;
  ClassDef::Methods methods;
  std::vector<std::unique_ptr<FunctionDefNode>> method_def_nodes;
  std::vector<ClassDefNode::Property> property_nodes;

  std::set<int64_t> property_names;
  std::set<int64_t> method_names;

  ClassDefHandle class_handle;

  {
    Store::Write writer(*store);
    class_handle = writer.make_class_definition();
  }

  ClassDefState::EnclosingClassHelper class_helper(class_state, class_handle);

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
      auto method_res = methods_block(method_names, methods, method_def_nodes);
      if (!method_res) {
        return NullOpt{};
      }
    } else {
      std::array<TokenType, 2> possible_types{
        {TokenType::keyword_properties, TokenType::keyword_methods}
      };

      add_error(make_error_expected_token_type(tok, possible_types));
      return NullOpt{};
    }
  }

  auto err = consume(TokenType::keyword_end);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  MatlabIdentifier name(string_registry->register_string(name_res.value()));
  ClassDef class_def(source_token, name, std::move(superclasses), std::move(properties), std::move(methods));

  {
    Store::Write writer(*store);
    writer.emplace_definition(class_handle, std::move(class_def));
  }

  auto class_node = std::make_unique<ClassDefNode>(source_token, class_handle,
    std::move(property_nodes), std::move(method_def_nodes));

  {
    Store::ReadMut reader(*store);
    auto& parent_scope = reader.at(parent_scope_handle);
    bool register_success = parent_scope.register_class(name, class_handle);
    if (!register_success) {
      add_error(make_error_duplicate_class_def(source_token));
      return NullOpt{};
    }
  }

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

Optional<FunctionAttributes> AstGenerator::method_attributes() {
  iterator.advance(); //  consume (

  FunctionAttributes attributes(class_state.enclosing_class());

  while (iterator.peek().type != TokenType::right_parens) {
    const auto& tok = iterator.peek();

    auto identifier_res = one_identifier();
    if (!identifier_res) {
      return NullOpt{};
    }

    const auto& attribute_name = identifier_res.value();

    if (!matlab::is_method_attribute(attribute_name)) {
      add_error(make_error_unrecognized_method_attribute(tok));
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
      add_error(make_error_invalid_access_attribute_value(tok));
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
    add_error(make_error_invalid_boolean_attribute_value(source_token));
    return NullOpt{};
  }

  return Optional<bool>(value == "true");
}

bool AstGenerator::methods_block(std::set<int64_t>& method_names,
                                 ClassDef::Methods& methods,
                                 std::vector<std::unique_ptr<FunctionDefNode>>& method_def_nodes) {
  iterator.advance(); //  consume methods

  //  methods (SetAccess = ...)
  if (iterator.peek().type == TokenType::left_parens) {
    auto attr_res = method_attributes();
    if (!attr_res) {
      return false;
    }

    push_function_attributes(attr_res.rvalue());
  } else {
    push_function_attributes(FunctionAttributes());
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
      default: {
        if (tok.type == TokenType::identifier || tok.type == TokenType::left_bracket) {
          auto res = method_declaration(tok, method_names, methods);
          if (!res) {
            return false;
          }

        } else if (tok.type == TokenType::keyword_function) {
          auto res = method_def(tok, method_names, methods, method_def_nodes);
          if (!res) {
            return false;
          }

        } else {
          std::array<TokenType, 3> possible_types{{
            TokenType::identifier, TokenType::left_bracket, TokenType::keyword_function
          }};

          add_error(make_error_expected_token_type(tok, possible_types));
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
                                      ClassDef::Methods& methods) {
  auto header_res = function_header();
  if (!header_res) {
    return false;
  }

  auto func_name = header_res.value().name;

  if (method_names.count(func_name.full_name()) > 0) {
    add_error(make_error_duplicate_method(source_token));
    return false;

  } else {
    method_names.emplace(func_name.full_name());
    FunctionDefHandle pending_def_handle;
    const auto& attrs = current_function_attributes();

    {
      Store::Write writer(*store);
      pending_def_handle = writer.make_function_declaration(header_res.rvalue(), attrs);
    }

    methods.emplace_back(pending_def_handle);
  }

  return true;
}

bool AstGenerator::method_def(const Token& source_token,
                              std::set<int64_t>& method_names,
                              mt::ClassDef::Methods& methods,
                              std::vector<std::unique_ptr<FunctionDefNode>>& method_def_nodes) {
  auto func_res = function_def();
  if (!func_res) {
    return false;
  }

  const auto& def_handle = func_res.value()->def_handle;
  Store::ReadConst reader(*store);
  const auto func_name = reader.at(def_handle).header.name;

  if (method_names.count(func_name.full_name()) > 0) {
    add_error(make_error_duplicate_method(source_token));
    return false;

  } else {
    method_names.emplace(func_name.full_name());
    methods.emplace_back(def_handle);
    method_def_nodes.emplace_back(std::move(func_res.rvalue()));
  }

  return true;
}

bool AstGenerator::properties_block(std::set<int64_t>& property_names,
                                    std::vector<ClassDef::Property>& properties,
                                    std::vector<ClassDefNode::Property>& property_nodes) {
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
      default: {
        auto prop_res = property(iterator.peek());
        if (!prop_res) {
          return false;
        }

        auto property_node = std::move(prop_res.rvalue());
        const auto prop_name = property_node.name;

        if (property_names.count(prop_name) > 0) {
          //  Duplicate property
          add_error(make_error_duplicate_class_property(tok));
          return false;
        } else {
          property_names.emplace(prop_name);
          properties.emplace_back(ClassDef::Property(MatlabIdentifier(prop_name)));
          property_nodes.emplace_back(std::move(property_node));
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

Optional<ClassDefNode::Property> AstGenerator::property(const Token& source_token) {
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

  auto name = string_registry->register_string(identifier_res.value());
  ClassDefNode::Property property(source_token, name, std::move(initializer));
  return Optional<ClassDefNode::Property>(std::move(property));
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
        if (is_end_terminated_function) {
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
      add_error(make_error_expected_token_type(tok, possible_types));
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
      add_error(make_error_reference_after_parens_reference_expr(tok));
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
  auto scope_handle = current_scope_handle();

  auto input_res = anonymous_function_input_parameters();
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

  auto params_err = check_anonymous_function_input_parameters_are_unique(source_token, input_identifiers);
  if (params_err) {
    add_error(params_err.rvalue());
    return NullOpt{};
  }

  auto node = std::make_unique<AnonymousFunctionExpr>(source_token, std::move(input_identifiers),
    body_res.rvalue(), scope_handle);
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
      add_error(make_error_expected_token_type(next, possible_types));
      return NullOpt{};
    }

    if (source_token.type == TokenType::left_parens && next.type != terminator) {
      //  (1, 2) or (1; 3) is not a valid expression.
      add_error(make_error_multiple_exprs_in_parens_grouping_expr(next));
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
    return Optional<ParseError>(make_error_expected_lhs(source_token));
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
    return Optional<ParseError>(make_error_expected_lhs(source_token));
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
    add_error(make_error_invalid_superclass_method_reference_expr(source_token));
    return NullOpt{};
  }

  auto* method_expr = static_cast<IdentifierReferenceExpr*>(method_reference_expr.get());

  int64_t subscript_end;
  const auto compound_identifier_components = method_expr->make_compound_identifier(&subscript_end);

  auto compound_identifier = string_registry->make_registered_compound_identifier(compound_identifier_components);
  MatlabIdentifier invoking_argument(compound_identifier);

  auto node = std::make_unique<PresumedSuperclassMethodReferenceExpr>(source_token,
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
      add_error(make_error_invalid_expr_token(tok));
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
      add_error(make_error_incomplete_expr(iterator.peek()));
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
    add_error(make_error_loop_control_flow_manipulator_outside_loop(source_token));
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
          add_error(make_error_duplicate_otherwise_in_switch_stmt(tok));
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
        add_error(make_error_expected_token_type(tok, possible_types));
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
    add_error(make_error_invalid_assignment_target(initial_token));
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
        add_error(make_error_expected_token_type(tok, &expected_type, 1));
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
    auto import_res = one_import(iterator.peek());
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

Optional<Import> AstGenerator::one_import(const mt::Token& source_token) {
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
      } else if (tok.type == TokenType::dot_asterisk) {
        //  Wildcard import a.b.c*
        iterator.advance();
        import_type = ImportType::wildcard;
        break;
      } else {
        std::array<TokenType, 2> possible_types{{TokenType::identifier, TokenType::period}};
        add_error(make_error_expected_token_type(tok, possible_types));
        return NullOpt{};
      }
    }

    if (tok.type == TokenType::identifier) {
      identifier_components.push_back(string_registry->register_string(tok.lexeme));
    }

    iterator.advance();
    expect_identifier = !expect_identifier;
  }

  const auto minimum_size = import_type == ImportType::wildcard ? 1 : 2;
  const int64_t num_components = identifier_components.size();

  if (num_components < minimum_size) {
    //  import; or import a;
    add_error(make_error_incomplete_import_stmt(source_token));
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
  const auto& dispatch_token = iterator.peek();
  Optional<BoxedTypeAnnot> annot_res;

  switch (dispatch_token.type) {
    case TokenType::left_bracket:
    case TokenType::identifier:
      annot_res = inline_type_annotation(dispatch_token);
      break;
    default:
      annot_res = type_annotation(dispatch_token);
  }

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

Optional<BoxedTypeAnnot> AstGenerator::type_annotation(const mt::Token& source_token) {
  switch (source_token.type) {
    case TokenType::keyword_begin:
      return type_begin(source_token);

    case TokenType::keyword_given:
      return type_given(source_token);

    case TokenType::keyword_let:
      return type_let(source_token);

    default:
      auto possible_types = type_annotation_block_possible_types();
      add_error(make_error_expected_token_type(source_token, possible_types));
      return NullOpt{};
  }
}

Optional<BoxedTypeAnnot> AstGenerator::type_let(const mt::Token& source_token) {
  iterator.advance();

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

  int64_t identifier = string_registry->register_string(ident_res.value());

  auto let_node = std::make_unique<TypeLet>(source_token, identifier, equal_to_type_res.rvalue());
  return Optional<BoxedTypeAnnot>(std::move(let_node));
}

Optional<BoxedTypeAnnot> AstGenerator::type_given(const mt::Token& source_token) {
  iterator.advance();

  const auto& ident_token = iterator.peek();
  auto identifier_res = type_variable_identifiers(ident_token);
  if (!identifier_res) {
    return NullOpt{};
  }

  if (identifier_res.value().empty()) {
    add_error(make_error_expected_non_empty_type_variable_identifiers(ident_token));
    return NullOpt{};
  }

  const auto& decl_token = iterator.peek();
  Optional<BoxedTypeAnnot> decl_res;

  switch (decl_token.type) {
    case TokenType::keyword_let:
      decl_res = type_let(decl_token);
      break;
    default: {
      std::array<TokenType, 1> possible_types{{TokenType::keyword_let}};
      add_error(make_error_expected_token_type(source_token, possible_types));
      return NullOpt{};
    }
  }

  if (!decl_res) {
    return NullOpt{};
  }

  auto identifiers = string_registry->register_strings(identifier_res.value());

  auto node = std::make_unique<TypeGiven>(source_token, std::move(identifiers), decl_res.rvalue());
  return Optional<BoxedTypeAnnot>(std::move(node));
}

Optional<std::vector<std::string_view>> AstGenerator::type_variable_identifiers(const mt::Token& source_token) {
  std::vector<std::string_view> type_variable_identifiers;

  if (source_token.type == TokenType::less) {
    iterator.advance();
    auto type_variable_res = identifier_sequence(TokenType::greater);
    if (!type_variable_res) {
      return NullOpt{};
    }
    type_variable_identifiers = type_variable_res.rvalue();

  } else if (source_token.type == TokenType::identifier) {
    iterator.advance();
    type_variable_identifiers.emplace_back(source_token.lexeme);

  } else {
    std::array<TokenType, 2> possible_types{{TokenType::less, TokenType::identifier}};
    add_error(make_error_expected_token_type(source_token, possible_types));
    return NullOpt{};
  }

  return Optional<std::vector<std::string_view>>(std::move(type_variable_identifiers));
}

Optional<BoxedTypeAnnot> AstGenerator::type_begin(const mt::Token& source_token) {
  iterator.advance();

  const bool is_exported = iterator.peek().type == TokenType::keyword_export;
  if (is_exported) {
    iterator.advance();
  }

  std::vector<BoxedTypeAnnot> contents;
  bool had_error = false;

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
          had_error = true;
          const auto possible_types = type_annotation_block_possible_types();
          iterator.advance_to_one(possible_types.data(), possible_types.size());
        }
      }
    }
  }

  if (had_error) {
    return NullOpt{};
  }

  auto err = consume(TokenType::keyword_end_type);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  auto node = std::make_unique<TypeBegin>(source_token, is_exported, std::move(contents));
  return Optional<BoxedTypeAnnot>(std::move(node));
}

Optional<BoxedType> AstGenerator::type(const mt::Token& source_token) {
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

  auto union_type_node = std::make_unique<UnionType>(std::move(union_type_members));
  return Optional<BoxedType>(std::move(union_type_node));
}

Optional<BoxedType> AstGenerator::one_type(const mt::Token& source_token) {
  switch (source_token.type) {
    case TokenType::left_bracket:
      return function_type(source_token);

    case TokenType::identifier:
      return scalar_type(source_token);

    default: {
      std::array<TokenType, 2> possible_types{{TokenType::left_bracket, TokenType::identifier}};
      add_error(make_error_expected_token_type(source_token, possible_types));
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
      add_error(make_error_expected_token_type(iterator.peek(), possible_types));
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

Optional<BoxedType> AstGenerator::function_type(const mt::Token& source_token) {
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

  auto type_node = std::make_unique<FunctionType>(source_token,
    output_res.rvalue(), input_res.rvalue());
  return Optional<BoxedType>(std::move(type_node));
}

Optional<BoxedType> AstGenerator::scalar_type(const mt::Token& source_token) {
  iterator.advance(); //  consume lexeme

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

  int64_t identifier = string_registry->register_string(source_token.lexeme);

  auto node = std::make_unique<ScalarType>(source_token, identifier, std::move(arguments));
  return Optional<BoxedType>(std::move(node));
}

std::array<TokenType, 3> AstGenerator::type_annotation_block_possible_types() {
  return std::array<TokenType, 3>{
    {TokenType::keyword_begin, TokenType::keyword_given, TokenType::keyword_let
  }};
}

std::array<TokenType, 5> AstGenerator::sub_block_possible_types() {
  return {
    {TokenType::keyword_if, TokenType::keyword_for, TokenType::keyword_while,
    TokenType::keyword_try, TokenType::keyword_switch}
  };
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

bool AstGenerator::is_within_function() const {
  return block_depths.function_def > 0;
}

bool AstGenerator::is_within_top_level_function() const {
  return block_depths.function_def == 1;
}

bool AstGenerator::parent_function_is_class_method() const {
  return is_within_class() && block_depths.function_def == 1;
}

bool AstGenerator::is_within_class() const {
  return block_depths.class_def > 0;
}

bool AstGenerator::is_within_methods() const {
  return block_depths.methods > 0;
}

bool AstGenerator::is_within_loop() const {
  return block_depths.for_stmt > 0 || block_depths.parfor_stmt > 0 || block_depths.while_stmt > 0;
}

void AstGenerator::push_scope() {
  Store::Write writer(*store);
  scope_handles.emplace_back(writer.make_matlab_scope(current_scope_handle()));
}

void AstGenerator::pop_scope() {
  assert(scope_handles.size() > 0 && "No scope to pop.");
  scope_handles.pop_back();
}

void AstGenerator::push_function_attributes(mt::FunctionAttributes&& attrs) {
  function_attributes.emplace_back(std::move(attrs));
}

const FunctionAttributes& AstGenerator::current_function_attributes() const {
  assert(!function_attributes.empty() && "No function attributes.");
  return function_attributes.back();
}

void AstGenerator::pop_function_attributes() {
  assert(function_attributes.size() > 0 && "No attributes to pop.");
  function_attributes.pop_back();
}

MatlabScopeHandle AstGenerator::current_scope_handle() const {
  return scope_handles.empty() ? MatlabScopeHandle() : scope_handles.back();
}

void AstGenerator::register_import(mt::Import&& import) {
  Store::ReadMut reader(*store);
  auto& scope = reader.at(current_scope_handle());
  scope.register_import(std::move(import));
}

Optional<ParseError> AstGenerator::consume(mt::TokenType type) {
  const auto& tok = iterator.peek();

  if (tok.type == type) {
    iterator.advance();
    return NullOpt{};
  } else {
    return Optional<ParseError>(make_error_expected_token_type(tok, &type, 1));
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

  return Optional<ParseError>(make_error_expected_token_type(tok, types, num_types));
}

Optional<ParseError>
AstGenerator::check_anonymous_function_input_parameters_are_unique(const Token& source_token,
                                                                   const std::vector<FunctionInputParameter>& inputs) const {
  std::set<int64_t> uniques;
  for (const auto& input : inputs) {
    if (input.is_ignored) {
      //  Input argument is ignored (~)
      continue;
    }

    auto orig_size = uniques.size();
    uniques.insert(input.name);

    if (orig_size == uniques.size()) {
      //  This value is not new.
      return Optional<ParseError>(make_error_duplicate_input_parameter_in_expr(source_token));
    }
  }
  return NullOpt{};
}

void AstGenerator::add_error(mt::ParseError&& err) {
  parse_errors.emplace_back(std::move(err));
}

ParseError AstGenerator::make_error_reference_after_parens_reference_expr(const mt::Token& at_token) const {
  const char* msg = "`()` indexing must appear last in an index expression.";
  return ParseError(text, at_token, msg);
}

ParseError AstGenerator::make_error_invalid_expr_token(const mt::Token& at_token) const {
  const auto type_name = "`" + std::string(to_string(at_token.type)) + "`";
  const auto message = std::string("Token ") + type_name + " is not permitted in expressions.";
  return ParseError(text, at_token, message);
}

ParseError AstGenerator::make_error_incomplete_expr(const mt::Token& at_token) const {
  return ParseError(text, at_token, "Expression is incomplete.");
}

ParseError AstGenerator::make_error_invalid_assignment_target(const mt::Token& at_token) const {
  return ParseError(text, at_token, "The expression on the left is not a valid target for assignment.");
}

ParseError AstGenerator::make_error_expected_lhs(const mt::Token& at_token) const {
  return ParseError(text, at_token, "Expected an expression on the left hand side.");
}

ParseError AstGenerator::make_error_multiple_exprs_in_parens_grouping_expr(const mt::Token& at_token) const {
  return ParseError(text, at_token,
    "`()` grouping expressions cannot contain multiple sub-expressions or span multiple lines.");
}

ParseError AstGenerator::make_error_duplicate_otherwise_in_switch_stmt(const Token& at_token) const {
  return ParseError(text, at_token, "Duplicate `otherwise` in `switch` statement.");
}

ParseError AstGenerator::make_error_expected_non_empty_type_variable_identifiers(const mt::Token& at_token) const {
  return ParseError(text, at_token, "Expected a non-empty list of identifiers.");
}

ParseError AstGenerator::make_error_duplicate_input_parameter_in_expr(const mt::Token& at_token) const {
  return ParseError(text, at_token, "Anonymous function contains a duplicate input parameter identifier.");
}

ParseError AstGenerator::make_error_loop_control_flow_manipulator_outside_loop(const mt::Token& at_token) const {
  return ParseError(text, at_token, "A `break` or `continue` statement cannot appear outside a loop statement.");
}

ParseError AstGenerator::make_error_invalid_function_def_location(const mt::Token& at_token) const {
  return ParseError(text, at_token, "A `function` definition cannot appear here.");
}

ParseError AstGenerator::make_error_duplicate_local_function(const mt::Token& at_token) const {
  return ParseError(text, at_token, "Duplicate local function.");
}

ParseError AstGenerator::make_error_incomplete_import_stmt(const mt::Token& at_token) const {
  return ParseError(text, at_token, "An `import` statement must include a compound identifier.");
}

ParseError AstGenerator::make_error_invalid_period_qualified_function_def(const Token& at_token) const {
  return ParseError(text, at_token, "A period-qualified `function` is invalid here.");
}

ParseError AstGenerator::make_error_duplicate_class_property(const Token& at_token) const {
  return ParseError(text, at_token, "Duplicate property.");
}

ParseError AstGenerator::make_error_duplicate_method(const Token& at_token) const {
  return ParseError(text, at_token, "Duplicate method.");
}

ParseError AstGenerator::make_error_duplicate_class_def(const Token& at_token) const {
  return ParseError(text, at_token, "Duplicate class definition.");
}

ParseError AstGenerator::make_error_invalid_superclass_method_reference_expr(const Token& at_token) const {
  return ParseError(text, at_token, "Invalid superclass method reference.");
}

ParseError AstGenerator::make_error_unrecognized_method_attribute(const Token& at_token) const {
  return ParseError(text, at_token, "Unrecognized method attribute.");
}

ParseError AstGenerator::make_error_invalid_boolean_attribute_value(const Token& at_token) const {
  return ParseError(text, at_token, "Expected attribute value to be one of `true` or `false`.");
}

ParseError AstGenerator::make_error_invalid_access_attribute_value(const Token& at_token) const {
  return ParseError(text, at_token, "Unrecognized access attribute value.");
}

ParseError AstGenerator::make_error_expected_token_type(const mt::Token& at_token, const mt::TokenType* types,
                                                        int64_t num_types) const {
  std::vector<std::string> type_strs;

  for (int64_t i = 0; i < num_types; i++) {
    std::string type_str = std::string("`") + to_string(types[i]) + "`";
    type_strs.emplace_back(type_str);
  }

  const auto expected_str = join(type_strs, ", ");
  std::string message = "Expected to receive one of these types: \n\n" + expected_str;
  message += (std::string("\n\nInstead, received: `") + to_string(at_token.type) + "`.");

  return ParseError(text, at_token, std::move(message));
}

}
