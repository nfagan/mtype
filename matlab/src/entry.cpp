#include "util.hpp"

#include "mt/scan.hpp"
#include "mt/unicode.hpp"
#include <iostream>

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
  if (nrhs > 0) {
    const auto str = mt::get_string_with_trap(prhs[0], "entry:get_string");
    
    const bool is_valid_unicode = mt::utf8::is_valid(str.c_str(), str.length());
    
    if (is_valid_unicode) {
      std::cout << "Is valid!" << std::endl;
    } else {
      std::cout << "Is not valid." << std::endl;
    }
  }
}