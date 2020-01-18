#pragma once

#include <string_view>

namespace mt {
  namespace matlab {
    const char** keywords(int* count);

    bool is_keyword(std::string_view str);
  }
}