#include "type_import_resolution.hpp"
#include "type_scope.hpp"
#include "../source_data.hpp"
#include "../Optional.hpp"
#include <cassert>

namespace mt {

namespace {
  ParseError make_parse_error(const TypeImportResolutionInstance& instance, const Token& token, const std::string& message) {
    auto maybe_source_data = instance.sources_by_token.lookup(token);
    assert(maybe_source_data);
    const auto& source_data = maybe_source_data.value();
    return ParseError(source_data.source, token, message, source_data.file_descriptor);
  }

  void add_error_duplicate_type_identifier(TypeImportResolutionInstance& instance, const Token& token) {
    auto msg = make_error_message_duplicate_type_identifier(token.lexeme);
    instance.add_error(make_parse_error(instance, token, msg));
  }
}

/*
 * TypeImportResolutionInstance
 */

TypeImportResolutionInstance::TypeImportResolutionInstance(const TokenSourceMap& sources_by_token) :
sources_by_token(sources_by_token) {
  //
}

void TypeImportResolutionInstance::add_error(const ParseError& err) {
  errors.push_back(err);
}

TypeImportResolutionResult maybe_resolve_type_imports(TypeScope* scope, TypeImportResolutionInstance& instance) {
  if (instance.visited.count(scope) > 0) {
    return TypeImportResolutionResult{true};
  }

  instance.visited.insert(scope);

  for (const auto& import : scope->imports) {
    auto res = maybe_resolve_type_imports(import.root, instance);
    if (!res.success) {
      return res;
    }

    const auto& exports = import.root->exports;
    const bool is_export = import.is_exported;

    for (const auto& exp : exports) {
      const auto maybe_already_registered = scope->registered_local_identifier(exp.first, is_export);

      if (maybe_already_registered) {
        const auto& ref = maybe_already_registered.value();
        if (ref->scope == exp.second->scope) {
          continue;

        } else {
          add_error_duplicate_type_identifier(instance, *ref->source_token);
          return TypeImportResolutionResult{false};
        }
      }

      scope->emplace_type(exp.first, exp.second, is_export);
    }
  }

  for (auto* child : scope->children) {
    auto res = maybe_resolve_type_imports(child, instance);
    if (!res.success) {
      return res;
    }
  }

  return TypeImportResolutionResult{true};
}

}