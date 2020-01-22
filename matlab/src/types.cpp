#include "types.hpp"

namespace mt {
namespace matlab {

bool is_logical(mxClassID id) {
  return id == mxLOGICAL_CLASS;
}

bool is_logical(const mxArray* array) {
  return is_logical(mxGetClassID(array));
}
  
bool is_numeric(mxClassID id) {
  switch (id) {
    case mxDOUBLE_CLASS:
    case mxSINGLE_CLASS:
    case mxINT8_CLASS:
    case mxUINT8_CLASS:
    case mxINT16_CLASS:
    case mxUINT16_CLASS:
    case mxINT32_CLASS:
    case mxUINT32_CLASS:
    case mxINT64_CLASS:
    case mxUINT64_CLASS:
      return true;
    default:
      return false;
  }
}
  
bool is_numeric(const mxArray* array) {
  return is_numeric(mxGetClassID(array));  
}

}
}