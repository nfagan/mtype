#pragma once

#include <array>

namespace mt {

namespace terminal_colors {
  extern const char* const red;
  extern const char* const green;
  extern const char* const yellow;
  extern const char* const blue;
  extern const char* const dflt;

  std::array<const char*, 5> all_colors();
}

}