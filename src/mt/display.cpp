#include "display.hpp"
#include <string>

namespace mt {

namespace style {
  const char* const red = "\x1B[31m";
  const char* const green = "\x1B[32m";
  const char* const yellow = "\x1B[33m";
  const char* const blue = "\x1B[34m";
  const char* const magenta = "\x1B[35m";
  const char* const cyan = "\x1B[36m";
  const char* const dflt = "\x1B[0m";
  const char* const underline = "\x1B[4m";
  const char* const bold = "\x1B[1m";
  const char* const italic = "\x1B[3m";
  const char* const faint = "\x1B[2m";

  std::array<const char*, 7> all_colors() {
    return {{red, green, yellow, blue, magenta, cyan, dflt}};
  }

  std::string color_code(uint8_t index) {
    return std::string("\x1B[38;5;") + std::to_string(index) + "m";
  }

  std::string color_code(uint8_t r, uint8_t g, uint8_t b) {
    using std::to_string;
    std::string code("\x1B[38;2;");
    code += to_string(r);
    code += ";";
    code += to_string(g);
    code += ";";
    code += to_string(b);
    code += "m";
    return code;
  }
}

}
