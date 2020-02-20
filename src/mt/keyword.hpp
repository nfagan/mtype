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

    bool is_method_attribute(std::string_view str);
    bool is_boolean_method_attribute(std::string_view str);
    bool is_boolean(std::string_view str);

    bool is_access_attribute(std::string_view str);
    bool is_abstract_attribute(std::string_view str);
    bool is_hidden_attribute(std::string_view str);
    bool is_sealed_attribute(std::string_view str);
    bool is_static_attribute(std::string_view str);

    bool is_access_attribute_value(std::string_view str);
    bool is_public_access_attribute_value(std::string_view str);
    bool is_private_access_attribute_value(std::string_view str);
    bool is_protected_access_attribute_value(std::string_view str);
    bool is_immutable_access_attribute_value(std::string_view str);
  }

  namespace typing {
    const char** keywords(int* count);
    bool is_keyword(std::string_view str);
    bool is_end_terminated(std::string_view kw);
  }
}