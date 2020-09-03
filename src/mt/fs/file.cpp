#include "file.hpp"
#include "path.hpp"
#include "../Optional.hpp"
#include "../config.hpp"
#include <fstream>

#if defined(MT_UNIX)
#include <sys/stat.h>
#endif

#if MT_IS_MSVC
#include <windows.h>
#endif

namespace mt::fs {

Optional<std::unique_ptr<std::string>> read_file(const FilePath& path) {
  std::ifstream ifs(path.str());
  if (!ifs) {
    return NullOpt{};
  }

  auto contents = std::make_unique<std::string>((std::istreambuf_iterator<char>(ifs)),
                                                (std::istreambuf_iterator<char>()));

  return Optional<std::unique_ptr<std::string>>(std::move(contents));
}

#if defined(MT_UNIX)
bool file_exists(const FilePath& path) {
  struct stat sb;
  const int status = stat(path.c_str(), &sb);
  if (status != 0) {
    return false;
  }
  return (sb.st_mode & S_IFMT) == S_IFREG;
}
#elif defined(MT_WIN)
bool file_exists(const FilePath& path) {
  return !(INVALID_FILE_ATTRIBUTES == GetFileAttributes(path.c_str()) && 
         GetLastError() == ERROR_FILE_NOT_FOUND);
}
#else
#error "Expected one of Unix or Windows for OS."
#endif

}
