#include "type_store.hpp"

namespace mt {

std::unordered_map<Type::Tag, double> TypeStore::type_distribution() const {
  std::unordered_map<Type::Tag, double> counts;
  for (const auto& ptr : types) {
    auto tag = ptr->tag;
    if (counts.count(tag) == 0) {
      counts[tag] = 0.0;
    }
    counts[tag] = counts[tag] + 1.0;
  }

  return counts;
}

}