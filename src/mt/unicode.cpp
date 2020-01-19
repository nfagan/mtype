#include "unicode.hpp"

namespace {

inline bool is_bit_set(uint8_t byte, uint8_t index) {
  return byte & ((uint8_t(1)) << index);
}

inline bool is_valid_byte(uint8_t byte, uint8_t n_leading_ones) {
  for (uint8_t i = 0; i < n_leading_ones; i++) {
    if (!is_bit_set(byte, 7 - i)) {
      return false;
    }
  }

  return !is_bit_set(byte, 7 - n_leading_ones);
}

inline uint8_t count_most_significant_set_bits(uint8_t byte, uint8_t max_n_bytes) {
  uint8_t n_bits = 0;

  for (uint8_t i = 0; i < max_n_bytes; i++) {
    if (is_bit_set(byte, 7 - i)) {
      n_bits++;
    } else {
      break;
    }
  }

  return n_bits;
}

}

int mt::utf8::count_code_units(const char* str, int64_t len) {
  uint8_t max_bytes = (len < 0) ?
                      0 : (len > int64_t(utf8::bytes_per_code_point)) ?
                          utf8::bytes_per_code_point : uint8_t(len);

  if (max_bytes == 0) {
    return 0;
  }

  auto byte0 = uint8_t(str[0]);

  //  Most significant bit is not set -> ascii character.
  if (!is_bit_set(byte0, 7)) {
    return 1;
  }

  uint8_t expected_n_bytes = count_most_significant_set_bits(byte0, max_bytes);

  //  First two bits should be set at minimum for valid multi-byte character.
  //  Or, we're expecting at least N bytes, but fewer than that many remain in the string.
  if (expected_n_bytes <= 1 || expected_n_bytes > len) {
    return 0;
  }

  //  Additional bytes need have the form: 10xxxxxx
  for (uint8_t i = 1; i < expected_n_bytes; i++) {
    auto byte = uint8_t(str[i]);

    if (!is_valid_byte(byte, 1)) {
      return 0;
    }
  }

  return expected_n_bytes;
}

bool mt::utf8::is_valid(const std::string& str) {
  return is_valid(str.c_str(), str.size());
}

bool mt::utf8::is_valid(const char* str, int64_t len) {
  int64_t index = 0;

  while (index < len) {
    int64_t n_units = count_code_units(str + index, len - index);

    if (n_units == 0) {
      return false;
    }

    index += n_units;
  }

  return true;
}