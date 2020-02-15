#include "file_resolution.hpp"
#include "string.hpp"
#include "store.hpp"

namespace mt {

std::unique_ptr<AbstractFileResolver> AbstractFileResolver::active_file_resolver = nullptr;

/*
 * AbstractFileResolver
 */

void AbstractFileResolver::set_active_file_resolver(std::unique_ptr<AbstractFileResolver> resolver) {
  active_file_resolver = std::move(resolver);
}

const std::unique_ptr<AbstractFileResolver>& AbstractFileResolver::get_active_file_resolver() {
  return active_file_resolver;
}

/*
 * FileResolver
 */

std::string FileResolver::which_external(int64_t identifier, const MatlabScopeHandle&) const {
  //  @TODO
  const auto identifier_str = std::string(string_registry->at(identifier));
  return identifier_str;
}

std::string FileResolver::which(int64_t, const MatlabScopeHandle& scope_handle) const {
  //  @TODO
  const auto& scope = scope_store->at(scope_handle);
  return std::to_string(scope.local_functions.size());
}

}
