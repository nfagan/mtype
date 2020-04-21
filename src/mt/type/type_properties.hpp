#pragma once

#include "types.hpp"

namespace mt {

class TypeProperties {
public:
  bool is_concrete_argument(const Type* arg) const;
  bool are_concrete_arguments(const TypePtrs & args) const;

private:
  bool is_concrete_argument(const types::Scheme& scheme) const;
  bool is_concrete_argument(const types::Abstraction& abstr) const;
  bool is_concrete_argument(const types::DestructuredTuple& tup) const;
  bool is_concrete_argument(const types::List& list) const;
  bool is_concrete_argument(const types::Class& class_type) const;
  bool is_concrete_argument(const types::Alias& alias) const;
};

}