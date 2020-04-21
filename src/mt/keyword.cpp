#include "keyword.hpp"
#include <cstring>

namespace mt {

namespace {
  bool is_keyword_impl(std::string_view str, const char** keywords, int num_keywords) {
    for (int i = 0; i < num_keywords; i++) {
      if (str == keywords[i]) {
        return true;
      }
    }

    return false;
  }
  bool begins_with_keyword_impl(std::string_view str, const char** keywords, int num_keywords) {
    for (int i = 0; i < num_keywords; i++) {
      const int64_t kw_len = std::strlen(keywords[i]);
      const int64_t str_len = str.size();

      if (str_len >= kw_len) {
        bool all_match = true;

        for (int64_t j = 0; j < kw_len; j++) {
          if (keywords[i][j] != str[j]) {
            all_match = false;
            break;
          }
        }

        if (all_match) {
          return true;
        }
      }
    }

    return false;
  }
}

const char** typing::keywords(int* count) {
  static const char* keywords[] = {
    "begin", "export", "given", "let", "namespace", "struct", "fun", "record", "declare",
    "constructor", "list"
  };

  *count = sizeof(keywords) / sizeof(keywords[0]);
  return keywords;
}

bool typing::is_keyword(std::string_view str) {
  int num_keywords;
  const auto kws = typing::keywords(&num_keywords);
  return is_keyword_impl(str, kws, num_keywords);
}

bool typing::is_end_terminated(std::string_view kw) {
  return kw == "begin" || kw == "namespace" || kw == "struct" || kw == "record";
}

const char** matlab::classdef_keywords(int* count) {
  static const char* keywords[] = {
    "methods", "properties", "events", "enumeration"
  };

  *count = sizeof(keywords) / sizeof(keywords[0]);
  return keywords;
}

const char** matlab::keywords(int* count) {
  //  help iskeyword
  static const char* keywords[] = {
    "break", "case", "catch", "classdef", "continue", "else", "elseif", "end", "for", "function",
    "global", "if", "otherwise", "parfor", "persistent", "return", "spmd", "switch", "try", "while",
    "import"
  };

  *count = sizeof(keywords) / sizeof(keywords[0]);
  return keywords;
}

const char** matlab::operator_method_names(int* count) {
  static const char* keywords[] = {
    "plus", "minus", "uminus", "uplus", "times", "mtimes", "rdivide", "ldivide", "mrdivide",
    "mldivide", "power", "mpower", "lt", "gt", "le", "ge", "ne", "eq", "and", "or", "not",
    "colon", "ctranspose", "transpose", "horzcat", "vertcat"
  };

  *count = sizeof(keywords) / sizeof(keywords[0]);
  return keywords;
}

bool matlab::is_keyword(std::string_view str) {
  int num_keywords;
  const auto kws = matlab::keywords(&num_keywords);
  return is_keyword_impl(str, kws, num_keywords);
}

bool matlab::begins_with_keyword(std::string_view str) {
  int num_keywords;
  const auto kws = matlab::keywords(&num_keywords);
  return begins_with_keyword_impl(str, kws, num_keywords);
}

bool matlab::is_classdef_keyword(std::string_view str) {
  int num_keywords;
  const auto kws = matlab::classdef_keywords(&num_keywords);
  return is_keyword_impl(str, kws, num_keywords);
}

bool matlab::is_end_terminated(std::string_view kw) {
  return kw == "classdef" || kw == "if" || kw == "for" || kw == "function" || kw == "parfor" ||
    kw == "spmd" || kw == "switch" || kw == "try" || kw == "while" || is_classdef_keyword(kw);
}

bool matlab::is_access_attribute_value(std::string_view str) {
  return str == "public" || str == "private" || str == "protected" || str == "immutable";
}

bool matlab::is_public_access_attribute_value(std::string_view str) {
  return str == "public";
}

bool matlab::is_private_access_attribute_value(std::string_view str) {
  return str == "private";
}

bool matlab::is_protected_access_attribute_value(std::string_view str) {
  return str == "protected";
}

bool matlab::is_immutable_access_attribute_value(std::string_view str) {
  return str == "immutable";
}

bool matlab::is_access_attribute(std::string_view str) {
  return str == "Access";
}

bool matlab::is_abstract_attribute(std::string_view str) {
  return str == "Abstract";
}

bool matlab::is_hidden_attribute(std::string_view str) {
  return str == "Hidden";
}

bool matlab::is_sealed_attribute(std::string_view str) {
  return str == "Sealed";
}

bool matlab::is_static_attribute(std::string_view str) {
  return str == "Static";
}

bool matlab::is_method_attribute(std::string_view str) {
  return str == "Abstract" || str == "Access" || str == "Hidden" || str == "Sealed" || str == "Static";
}

bool matlab::is_boolean_method_attribute(std::string_view str) {
  return str == "Abstract" || str == "Hidden" || str == "Sealed" || str == "Static";
}

bool matlab::is_boolean(std::string_view str) {
  return str == "true" || str == "false";
}

bool matlab::is_operator_method_name(std::string_view str) {
  int num_keywords;
  const auto kws = matlab::operator_method_names(&num_keywords);
  return is_keyword_impl(str, kws, num_keywords);
}

bool is_end_terminated(std::string_view kw) {
  return matlab::is_end_terminated(kw) || typing::is_end_terminated(kw);
}

}
