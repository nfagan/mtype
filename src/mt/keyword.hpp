#pragma once

#include <string_view>

namespace mt {
  bool is_end_terminated(std::string_view kw);

  namespace matlab {
    //  Keywords in a classdef context.
    const char** classdef_keywords(int* count);

    //  Keywords in any context.
    const char** keywords(int* count);

    bool is_keyword(std::string_view str);
    bool is_classdef_keyword(std::string_view str);
    bool is_end_terminated(std::string_view kw);
  }

  namespace typing {
    const char** keywords(int* count);
    bool is_keyword(std::string_view str);
    bool is_end_terminated(std::string_view kw);
  }
}