#include "path.hpp"
#include "../config.hpp"
#include "../string.hpp"
#include "../character.hpp"
#include <cstring>
#include <functional>

namespace mt {

namespace {
#if defined(MT_UNIX)
  const Character separator_character('/');
#else
  const Character separator_character('\\');
#endif
}

#if defined(MT_UNIX)
const char* const fs::separator = "/";
#else
const char* const fs::separator = "\\";
#endif

std::size_t FilePath::Hash::operator()(const FilePath& a) const {
  return std::hash<std::string>{}(a.component);
}

FilePath fs::join(const FilePath& a, const FilePath& b) {
  return FilePath(a.str() + fs::separator + b.str());
}

FilePath fs::directory_name(const FilePath& a) {
  const auto& a_str = a.str();
  auto rev = utf8_reverse(a_str);

  auto first_slash = find_character(rev.c_str(), rev.size(), separator_character);

  if (first_slash == -1) {
    return FilePath();
  }

  if (first_slash == 0 && a_str.size() == std::size_t(separator_character.count_units())) {
    // a == '/'
    return FilePath(fs::separator);
  }

  auto substr = a_str.substr(0, a_str.size() - (first_slash + 1));
  return FilePath(substr);
}

std::string fs::package_name(const FilePath& path) {
  const auto& str = path.str();
  auto splt = split(str, separator_character);
  std::string package;
  bool first_non_empty_dir = true;

  for (int64_t i = splt.size()-1; i >= 0; i--) {
    auto component = std::string(splt[i]);

    if (component.empty()) {
      continue;

    } else if (first_non_empty_dir && component[0] == '@') {
      //  E.g. for file +a/@b/b.m, skip the @b directory, because b.m should be
      //  registered as a.b().
      first_non_empty_dir = false;
      continue;

    } else if (component[0] != '+') {
      break;
    }

    component += ".";
    package.insert(0, component.data()+1);
  }

  if (!package.empty() && package[package.size()-1] == '.') {
    package.erase(package.size()-1);
  }

  return package;
}

}
