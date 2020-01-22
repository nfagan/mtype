#pragma once

#include "mex.h"
#include <type_traits>
#include <cstdint>

namespace mt {
namespace matlab {
  
bool is_numeric(const mxArray* array);
bool is_numeric(mxClassID id);

bool is_logical(const mxArray* array);
bool is_logical(mxClassID id);
  
using single = float;
  
template <typename T>
struct is_float_t : public std::false_type {};

template<>
struct is_float_t<double> : public std::true_type {};
template<>
struct is_float_t<single> : public std::true_type {};

template <typename T>
struct is_numeric_t : public std::false_type {};

template<>
struct is_numeric_t<double> : public std::true_type {};
template<>
struct is_numeric_t<single> : public std::true_type {};

template<>
struct is_numeric_t<uint8_t> : public std::true_type {};
template<>
struct is_numeric_t<int8_t> : public std::true_type {};
template<>
struct is_numeric_t<uint16_t> : public std::true_type {};
template<>
struct is_numeric_t<int16_t> : public std::true_type {};
template<>
struct is_numeric_t<int> : public std::true_type {};
template<>
struct is_numeric_t<unsigned int> : public std::true_type {};
template<>
struct is_numeric_t<int64_t> : public std::true_type {};
template<>
struct is_numeric_t<uint64_t> : public std::true_type {};
  
// template <typename T>
// struct UnderlyingDataAccessor {
//   static typename std::enable_if<std::is_same<T, bool>::value, bool*> access(const mxArray* data) {
//     return (bool*) mxGetLogicals(data);
//   }
//   template <typename U = typename std::enable_if<std::is_same<U, int>::value, int> = 0>
//   static T* access(const mxArray* data) {
//     return (T*) mxGetData(data);
//   }
// };

}
}