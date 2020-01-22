#include "util.hpp"

#include "mt/mt.hpp"

#include <iostream>
#include <cassert>

namespace {
struct Inputs {
  bool show_parse_errors = true;
  bool show_ast = false;
  bool show_tokens = false;
};

Inputs parse_inputs(int nrhs, const mxArray *prhs[]) {
  Inputs inputs;
  
  if (nrhs > 1) {
    inputs.show_parse_errors = mt::bool_convertible_or_default(prhs[1], inputs.show_parse_errors);
  }
  
  if (nrhs > 2) {
    inputs.show_ast = mt::bool_convertible_or_default(prhs[2], inputs.show_ast);
  }
  
  if (nrhs > 3) {
    inputs.show_tokens = mt::bool_convertible_or_default(prhs[3], inputs.show_tokens);
  }
  
  return inputs;
}

}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
  if (nrhs == 0) {
    mexErrMsgIdAndTxt("entry:get_string", "Not enough input arguments.");
    return;
  }
  
  const auto inputs = parse_inputs(nrhs, prhs);
  
  //  Initialize to error state.
  plhs[0] = mxCreateLogicalScalar(false);

  const auto str = mt::get_string_with_trap(prhs[0], "entry:get_string");
  const bool is_valid_unicode = mt::utf8::is_valid(str.c_str(), str.length());
  
  if (!is_valid_unicode) {
    if (inputs.show_parse_errors) {
      std::cout << "Input is not valid unicode." << std::endl;
    }
    return;
  }

  mt::Scanner scanner;
  mt::AstGenerator ast_generator;

  auto scan_res = scanner.scan(str);

  if (!scan_res) {
    if (inputs.show_parse_errors) {
      for (const auto& err : scan_res.error.errors) {
        std::cout << err.message << std::endl;
      }
    }
    return;
  }
  
  auto& scan_info = scan_res.value;
  auto insert_res = insert_implicit_expr_delimiters_in_groupings(scan_info.tokens, str);
  
  if (insert_res) {
    if (inputs.show_parse_errors) {
      insert_res.value().show();
    }
    return;
  }
  
  if (inputs.show_tokens) {
    for (const auto& tok : scan_info.tokens) {
      std::cout << tok << std::endl;
    }
  }

  const bool end_terminated = scan_info.functions_are_end_terminated;
  auto parse_res = ast_generator.parse(scan_info.tokens, str, end_terminated);

  if (!parse_res) {
    if (inputs.show_parse_errors) {
      for (const auto& err : parse_res.error) {
        err.show();
      }
    }
    return;
  }
  
  //  Success.
  *mxGetLogicals(plhs[0]) = true;

  if (inputs.show_ast) {
    mt::StringVisitor visitor;
    visitor.parenthesize_exprs = true;
    std::cout << parse_res.value->accept(visitor) << std::endl;
  }
}