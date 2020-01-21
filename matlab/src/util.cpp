#include "util.hpp"

bool mt::bool_or_default(const mxArray* in_array, bool default_value) {
  if (mxGetClassID(in_array) != mxLOGICAL_CLASS || 
      mxGetNumberOfElements(in_array) != 1) {
    return default_value;
  } else {
    return mxIsLogicalScalarTrue(in_array);
  }
}

std::string mt::get_string_with_trap(const mxArray* in_str, const char* id) {
  if (mxGetClassID(in_str) != mxCHAR_CLASS) {
    mexErrMsgIdAndTxt(id, "Input must be char.");
    return "";
  }
  
#if 1
  uint64_t sz = mxGetNumberOfElements(in_str);
  size_t strlen = sz + 1;
  char* str = (char*) std::malloc(strlen);

  if (str == nullptr) {
    mexErrMsgIdAndTxt(id, "String memory allocation failed.");
    return "";
  }

  int result = mxGetString(in_str, str, strlen);

  if (result != 0) {
    std::free(str);
    mexErrMsgIdAndTxt(id, "String copy failed.");
    return "";
  }

  std::string res(str);
  std::free(str);
  
#else
  char* str = mxArrayToUTF8String(in_str);
  
  if (str == nullptr) {
    mexErrMsgIdAndTxt(id, "String memory allocation failed.");
    return "";
  }
  
  std::string res(str);  
  mxFree(str);
#endif

  return res;
}