#pragma once

#include "ast.hpp"
#include <string>
#include <string_view>

namespace mt {

class StringRegistry;

class AbstractFileResolver {
public:
  AbstractFileResolver() = default;
  virtual ~AbstractFileResolver() = default;

  virtual std::string which(int64_t identifier, BoxedMatlabScope scope) const = 0;
  virtual std::string which_external(int64_t identifier, BoxedMatlabScope scope) const = 0;

  static void set_active_file_resolver(std::unique_ptr<AbstractFileResolver> resolver);
  static const std::unique_ptr<AbstractFileResolver>& get_active_file_resolver();

private:
  static std::unique_ptr<AbstractFileResolver> active_file_resolver;
};

class FileResolver : AbstractFileResolver {
public:
  FileResolver(const StringRegistry* string_registry,
               std::string current_directory,
               std::vector<std::string> path_directories) :
  string_registry(string_registry),
  current_directory(std::move(current_directory)),
  path_directories(std::move(path_directories)) {
    //
  }
  ~FileResolver() override = default;

  std::string which(int64_t identifier, BoxedMatlabScope scope) const override;
  std::string which_external(int64_t identifier, BoxedMatlabScope scope) const override;

private:
  const StringRegistry* string_registry;
  std::string current_directory;
  std::vector<std::string> path_directories;
};

}