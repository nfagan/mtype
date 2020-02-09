#pragma once

#include <cstdint>
#include <string>

namespace mt {
  namespace utf8 {
    static constexpr uint8_t bytes_per_code_point = 4;

    int count_code_units(const char* str, int64_t len);
    int count_code_units(const std::string& str);

    int64_t count_code_points(const char* str, int64_t len);
    int64_t count_code_points(const std::string& str);

    bool is_valid(const char* str, int64_t len);
    bool is_valid(const std::string& str);
  }
}