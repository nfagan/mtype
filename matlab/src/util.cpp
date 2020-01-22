#include "util.hpp"
#include "types.hpp"

namespace mt {
  
namespace {
template <typename T>
bool strict_bool_convertible_value_or_default(const mxArray* in_array, bool default_value) {
  T element = *((T*)(mxGetData(in_array)));
  if (element == T(1)) {
    return true;
  } else if (element == T(0)) {
    return false;
  } else {
    return default_value;
  }
}
}
  
bool bool_convertible_or_default(const mxArray* in_array, bool default_value) {
  if (mxGetNumberOfElements(in_array) != 1) {
    return default_value;
  }
  if (matlab::is_logical(in_array)) {
    return mxIsLogicalScalarTrue(in_array);
    
  } else if (!matlab::is_numeric(in_array)) {
    return default_value;
  }
  
  const auto id = mxGetClassID(in_array);  
  switch (id) {
    case mxSINGLE_CLASS:
      return strict_bool_convertible_value_or_default<float>(in_array, default_value);
      
    case mxDOUBLE_CLASS:
      return strict_bool_convertible_value_or_default<double>(in_array, default_value);
      
    case mxUINT8_CLASS:
      return strict_bool_convertible_value_or_default<uint8_t>(in_array, default_value);
      
    case mxINT8_CLASS:
      return strict_bool_convertible_value_or_default<int8_t>(in_array, default_value);
      
    case mxUINT16_CLASS:
      return strict_bool_convertible_value_or_default<uint16_t>(in_array, default_value);
      
    case mxINT16_CLASS:
      return strict_bool_convertible_value_or_default<int16_t>(in_array, default_value);
      
    case mxUINT32_CLASS:
      return strict_bool_convertible_value_or_default<uint32_t>(in_array, default_value);
      
    case mxINT32_CLASS:
      return strict_bool_convertible_value_or_default<int32_t>(in_array, default_value);
      
    default:
      return default_value;
  }
}

bool bool_or_default(const mxArray* in_array, bool default_value) {
  if (mxGetClassID(in_array) != mxLOGICAL_CLASS || 
      mxGetNumberOfElements(in_array) != 1) {
    return default_value;
  } else {
    return mxIsLogicalScalarTrue(in_array);
  }
}

std::string get_string_with_trap(const mxArray* in_str, const char* id) {
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

}