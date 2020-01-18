#pragma once

#include <string_view>

namespace mt {
  bool is_end_terminated(std::string_view kw);

  namespace matlab {
    const char** keywords(int* count);
    bool is_keyword(std::string_view str);
    bool is_end_terminated(std::string_view kw);
  }

  namespace typing {
    const char** keywords(int* count);
    bool is_keyword(std::string_view str);
    bool is_end_terminated(std::string_view kw);
  }
}