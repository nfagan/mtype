#pragma once

#include "mex.h"
#include <string>

namespace mt {
  std::string get_string_with_trap(const mxArray* in_str, const char* id);
}