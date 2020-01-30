#include "file_resolution.hpp"
#include "string.hpp"

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

std::string FileResolver::which_external(int64_t identifier, BoxedMatlabScope scope) const {
  //  @TODO
  const auto identifier_str = string_registry->at(identifier);

  for (const auto& import : scope->wildcard_imports) {
    std::string import_name = string_registry->make_compound_identifier(import.identifier_components);
    import_name += ".";
    import_name += identifier_str;
  }

  return "";
}

std::string FileResolver::which(int64_t, BoxedMatlabScope) const {
  //  @TODO
  return "";
}

}
