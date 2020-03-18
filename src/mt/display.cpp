#include "display.hpp"

namespace mt {

namespace terminal_colors {
  const char* const red = "\x1B[31m";
  const char* const green = "\x1B[32m";
  const char* const yellow = "\x1B[33m";
  const char* const blue = "\x1B[34m";
  const char* const dflt = "\x1B[0m";

  std::array<const char*, 5> all_colors() {
    return {{red, green, yellow, blue, dflt}};
  }
}

}
