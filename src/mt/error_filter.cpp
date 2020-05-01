#include "error_filter.hpp"
#include "fs.hpp"

namespace mt {

bool ErrorFilter::matches_identifier(const FilePath& file_path) const {
  auto maybe_package = fs::package_name(fs::directory_name(file_path));
  auto file_name = fs::sans_extension(fs::file_name(file_path));
  auto identifier = maybe_package.empty() ?
    file_name.str() :
    maybe_package + "." + file_name.str();

  return identifiers.count(identifier) > 0;
}

}
