#include "keyword.hpp"

namespace mt {

const char** matlab::keywords(int* count) {
  //  help iskeyword
  static const char* keywords[] = {
    "break", "case", "catch", "classdef", "continue", "else", "elseif", "end", "for", "function", "global",
    "if", "otherwise", "parfor", "persistent", "return", "spmd", "switch", "try", "while"
  };

  *count = sizeof(keywords) / sizeof(keywords[0]);
  return keywords;
}

bool matlab::is_keyword(std::string_view str) {
  int num_keywords;
  auto kws = keywords(&num_keywords);

  for (int i = 0; i < num_keywords; i++) {
    if (str == kws[i]) {
      return true;
    }
  }

  return false;
}

}
