#include "identifier.hpp"
#include <functional>

namespace mt {

std::size_t MatlabIdentifier::Hash::operator()(const MatlabIdentifier& k) const {
  using std::hash;
  return hash<int64_t>()(k.name);
}

}