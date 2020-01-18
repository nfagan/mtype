#include "util.hpp"

#include "mt/scan.hpp"
#include "mt/unicode.hpp"
#include "mt/string.hpp"

#include <iostream>

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
  if (nrhs == 0) {
    std::cout << "Not enough input arguments." << std::endl;
    return;
  }

  const auto str = mt::get_string_with_trap(prhs[0], "entry:get_string");
  const bool is_valid_unicode = mt::utf8::is_valid(str.c_str(), str.length());

  if (is_valid_unicode) {      
    mt::Scanner scanner;
    auto res = scanner.scan(str);

    if (res) {
      for (const auto& tok : res.value) {
        std::cout << tok << std::endl;
      }
    } else {
      for (const auto& err : res.error.errors) {
        std::cout << err.message << std::endl;
      }
    }

  } else {
    std::cout << "Input is not valid unicode." << std::endl;
  }
}