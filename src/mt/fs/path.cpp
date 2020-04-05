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

}
