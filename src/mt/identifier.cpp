#include "identifier.hpp"
#include <functional>

namespace mt {

/*
 * MatlabIdentifier
 */

std::size_t MatlabIdentifier::Hash::operator()(const MatlabIdentifier& k) const noexcept {
  return std::hash<int64_t>()(k.name);
}

/*
 * TypeIdentifier
 */

std::size_t TypeIdentifier::Hash::operator()(const TypeIdentifier& id) const noexcept {
  return std::hash<int64_t>{}(id.name);
}

}