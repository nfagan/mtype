#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace mt {

namespace style {
  extern const char* const red;
  extern const char* const green;
  extern const char* const yellow;
  extern const char* const blue;
  extern const char* const magenta;
  extern const char* const cyan;
  extern const char* const dflt;

  extern const char* const underline;
  extern const char* const bold;
  extern const char* const italic;
  extern const char* const faint;

  std::array<const char*, 7> all_colors();
  std::string color_code(uint8_t index);
  std::string color_code(uint8_t r, uint8_t g, uint8_t b);
}

}