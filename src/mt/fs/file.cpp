#include "file.hpp"
#include "path.hpp"
#include "../Optional.hpp"
#include <fstream>

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

}
