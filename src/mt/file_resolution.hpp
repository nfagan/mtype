#pragma once

#include "ast.hpp"
#include <string>
#include <string_view>

namespace mt {

class StringRegistry;
class ScopeStore;

class AbstractFileResolver {
public:
  AbstractFileResolver(const ScopeStore* scope_store) : scope_store(scope_store) {
    //
  }
  virtual ~AbstractFileResolver() = default;

  virtual std::string which(int64_t identifier, const MatlabScopeHandle& scope_handle) const = 0;
  virtual std::string which_external(int64_t identifier, const MatlabScopeHandle& scope_handle) const = 0;

  static void set_active_file_resolver(std::unique_ptr<AbstractFileResolver> resolver);
  static const std::unique_ptr<AbstractFileResolver>& get_active_file_resolver();

protected:
  const ScopeStore* scope_store;

private:
  static std::unique_ptr<AbstractFileResolver> active_file_resolver;
};

class FileResolver : AbstractFileResolver {
public:
  FileResolver(const ScopeStore* scope_store,
               const StringRegistry* string_registry,
               std::string current_directory,
               std::vector<std::string> path_directories) :
  AbstractFileResolver(scope_store),
  string_registry(string_registry),
  current_directory(std::move(current_directory)),
  path_directories(std::move(path_directories)) {
    //
  }
  ~FileResolver() override = default;

  std::string which(int64_t identifier, const MatlabScopeHandle& scope_handle) const override;
  std::string which_external(int64_t identifier, const MatlabScopeHandle& scope_handle) const override;

private:
  const StringRegistry* string_registry;
  std::string current_directory;
  std::vector<std::string> path_directories;
};

}