#include "keyword.hpp"

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
}

const char** typing::keywords(int* count) {
  static const char* keywords[] = {
    "begin", "export", "given", "let", "namespace", "struct"
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
  return kw == "begin" || kw == "namespace" || kw == "struct";
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

bool matlab::is_keyword(std::string_view str) {
  int num_keywords;
  const auto kws = matlab::keywords(&num_keywords);
  return is_keyword_impl(str, kws, num_keywords);
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

bool is_end_terminated(std::string_view kw) {
  return matlab::is_end_terminated(kw) || typing::is_end_terminated(kw);
}

}
