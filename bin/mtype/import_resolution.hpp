#pragma once

#include "mt/error.hpp"
#include <unordered_set>

namespace mt {

struct TypeScope;
struct Token;
class TypeStore;
class TokenSourceMap;

struct TypeImportResolutionInstance {
  TypeImportResolutionInstance(const TokenSourceMap& sources_by_token);

  void add_error(const ParseError& err);

  const TokenSourceMap& sources_by_token;
  std::unordered_set<const TypeScope*> visited;
  ParseErrors errors;
};

struct TypeImportResolutionResult {
  bool success;
};

TypeImportResolutionResult maybe_resolve_type_imports(TypeScope* scope, TypeImportResolutionInstance& instance);

}