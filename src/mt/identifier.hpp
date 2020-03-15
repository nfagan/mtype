#pragma once

#include <cstdint>
#include <cstddef>

namespace mt {

class MatlabIdentifier {
public:
  struct Hash {
    std::size_t operator()(const MatlabIdentifier& k) const noexcept;
  };

public:
  MatlabIdentifier() : MatlabIdentifier(-1, 0) {
    //
  }

  explicit MatlabIdentifier(int64_t name) : MatlabIdentifier(name, 1) {
    //
  }

  MatlabIdentifier(int64_t name, int num_components) :
    name(name), num_components(num_components) {
    //
  }

  friend bool operator==(const MatlabIdentifier& a, const MatlabIdentifier& b) {
    return a.name == b.name;
  }

  friend bool operator!=(const MatlabIdentifier& a, const MatlabIdentifier& b) {
    return a.name != b.name;
  }

  friend bool operator<(const MatlabIdentifier& a, const MatlabIdentifier& b) {
    return a.name < b.name;
  }

  bool is_valid() const {
    return size() > 0;
  }

  bool is_compound() const {
    return num_components > 1;
  }

  int size() const {
    return num_components;
  }

  int64_t full_name() const {
    return name;
  }

private:
  int64_t name;
  int num_components;
};

}