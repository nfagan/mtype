#include "util.hpp"

#include "mt/scan.hpp"
#include "mt/ast_gen.hpp"
#include "mt/unicode.hpp"
#include "mt/string.hpp"

#include <iostream>
#include <cassert>

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
  if (nrhs == 0) {
    std::cout << "Not enough input arguments." << std::endl;
    return;
  }

  const auto str = mt::get_string_with_trap(prhs[0], "entry:get_string");
  const bool is_valid_unicode = mt::utf8::is_valid(str.c_str(), str.length());

  if (is_valid_unicode) {
    mt::Scanner scanner;
    mt::AstGenerator ast_generator;
    
    auto scan_res = scanner.scan(str);
    
    if (!scan_res) {
      for (const auto& err : scan_res.error.errors) {
        std::cout << err.message << std::endl;
      }
      return;
    } else {
      for (const auto& tok : scan_res.value) {
        std::cout << tok << std::endl;
      }
    }
    
    auto parse_res = ast_generator.parse(scan_res.value, str);
    
    if (!parse_res) {
      for (const auto& err : parse_res.error) {
        err.show();
      }
    }

  } else {
    std::cout << "Input is not valid unicode." << std::endl;
  }
}