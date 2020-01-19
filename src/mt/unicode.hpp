#pragma once

#include <cstdint>
#include <string>

namespace mt {
  namespace utf8 {
    static constexpr uint8_t bytes_per_code_point = 4;

    int count_code_units(const char* str, int64_t len);
    bool is_valid(const char* str, int64_t len);
    bool is_valid(const std::string& str);
  }
}