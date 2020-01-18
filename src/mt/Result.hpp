#pragma once

#include <utility>

namespace mt {

template <typename E, typename V>
class Result {
public:
  Result() : is_error(false) {
    //
  }

  explicit Result(E&& err) : error(std::move(err)), is_error(true) {
    //
  }

  explicit Result(V&& value, int = 0) : value(std::move(value)), is_error(false) {
    //
  }

  // NOLINTNEXTLINE
  operator bool() const {
    return !is_error;
  }

  ~Result() = default;

  E error;
  V value;
  bool is_error;
};

template <typename E, typename V, typename... Args>
inline Result<E, V> make_error(Args&&... args) {
  E err(args...);
  return Result<E, V>(std::move(err));
}

template <typename E, typename V, typename... Args>
inline Result<E, V> make_success(Args&&... args) {
  V value(args...);
  return Result<E, V>(std::move(value), 1);
}

}