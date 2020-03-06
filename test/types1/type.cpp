#include "type.hpp"
#include "typing.hpp"
#include <cassert>

namespace mt {

/*
 * Application
 */

bool types::Abstraction::operator==(const types::Abstraction& other) const {
  if (type != other.type) {
    return false;
  }
  if ((type == Type::binary_operator && binary_operator != other.binary_operator) ||
      (type == Type::unary_operator && unary_operator != other.unary_operator)) {
    return false;
  }
  if (inputs.size() != other.inputs.size() || outputs.size() != other.outputs.size()) {
    return false;
  }
  for (int64_t i = 0; i < inputs.size(); i++) {
    if (inputs[i] != other.inputs[i]) {
      return false;
    }
  }
  for (int64_t i = 0; i < outputs.size(); i++) {
    if (outputs[i] != other.outputs[i]) {
      return false;
    }
  }
  return true;
}

namespace {
template <typename T>
inline int compare_less(const std::vector<T>& a, const std::vector<T>& b) {
  const int64_t size_a = a.size();
  const int64_t size_b = b.size();

  if (size_a < size_b) {
    return -1;
  } else if (size_a > size_b) {
    return 1;
  } else {
    for (int64_t i = 0; i < size_a; i++) {
      if (a[i] < b[i]) {
        return -1;
      } else if (a[i] > b[i]) {
        return 1;
      }
    }
  }

  return 0;
}
}

bool types::Abstraction::Less::operator()(const types::Abstraction& a, const types::Abstraction& b) const {
  const auto a_type = static_cast<uint8_t>(a.type);
  const auto b_type = static_cast<uint8_t>(b.type);

  if (a_type < b_type) {
    return true;
  } else if (a_type > b_type) {
    return false;
  }

  if (a.type == Type::binary_operator) {
    if (a.binary_operator < b.binary_operator) {
      return true;
    } else if (a.binary_operator > b.binary_operator) {
      return false;
    }
  } else if (a.type == Type::unary_operator) {
    if (a.unary_operator < b.unary_operator) {
      return true;
    } else if (a.unary_operator > b.unary_operator) {
      return false;
    }
  } else {
    assert(false && "Function type not properly handled.");
  }

  const auto arg_res = compare_less(a.inputs, b.inputs);
  if (arg_res == -1) {
    return true;
  } else if (arg_res == 1) {
    return false;
  }

  const auto output_res = compare_less(a.outputs, b.outputs);
  if (output_res == -1) {
    return true;
  } else if (output_res == 1) {
    return false;
  }

  return false;
}

const char* to_string(DebugType::Tag tag) {
  using Tag = DebugType::Tag;

  switch (tag) {
    case Tag::null:
      return "null";
    case Tag::variable:
      return "variable";
    case Tag::scalar:
      return "scalar";
    case Tag::abstraction:
      return "abstraction";
    case Tag::union_type:
      return "union_type";
    case Tag::tuple:
      return "tuple";
    default:
      assert(false && "Unhandled.");
  }
}

std::ostream& operator<<(std::ostream& stream, DebugType::Tag tag) {
  stream << to_string(tag);
  return stream;
}

void DebugType::default_construct() noexcept {
  switch (tag) {
    case Tag::null:
      break;
    case Tag::variable:
      new (&variable) types::Variable();
      break;
    case Tag::scalar:
      new (&scalar) types::Scalar();
      break;
    case Tag::abstraction:
      new (&abstraction) types::Abstraction();
      break;
    case Tag::union_type:
      new (&union_type) types::Union();
      break;
    case Tag::tuple:
      new (&tuple) types::Tuple();
      break;
  }
}

void DebugType::move_construct(DebugType&& other) noexcept {
  switch (tag) {
    case Tag::null:
      break;
    case Tag::variable:
      new (&variable) types::Variable(std::move(other.variable));
      break;
    case Tag::scalar:
      new (&scalar) types::Scalar(std::move(other.scalar));
      break;
    case Tag::abstraction:
      new (&abstraction) types::Abstraction(std::move(other.abstraction));
      break;
    case Tag::union_type:
      new (&union_type) types::Union(std::move(other.union_type));
      break;
    case Tag::tuple:
      new (&tuple) types::Tuple(std::move(other.tuple));
      break;
  }
}

void DebugType::copy_construct(const DebugType& other) {
  switch (tag) {
    case Tag::null:
      break;
    case Tag::variable:
      new (&variable) types::Variable(other.variable);
      break;
    case Tag::scalar:
      new (&scalar) types::Scalar(other.scalar);
      break;
    case Tag::abstraction:
      new (&abstraction) types::Abstraction(other.abstraction);
      break;
    case Tag::union_type:
      new (&union_type) types::Union(other.union_type);
      break;
    case Tag::tuple:
      new (&tuple) types::Tuple(other.tuple);
      break;
  }
}

void DebugType::copy_assign(const DebugType& other) {
  switch (tag) {
    case Tag::null:
      break;
    case Tag::variable:
      variable = other.variable;
      break;
    case Tag::scalar:
      scalar = other.scalar;
      break;
    case Tag::abstraction:
      abstraction = other.abstraction;
      break;
    case Tag::union_type:
      union_type = other.union_type;
      break;
    case Tag::tuple:
      tuple = other.tuple;
      break;
  }
}

void DebugType::move_assign(DebugType&& other) {
  switch (tag) {
    case Tag::null:
      break;
    case Tag::variable:
      variable = std::move(other.variable);
      break;
    case Tag::scalar:
      scalar = std::move(other.scalar);
      break;
    case Tag::abstraction:
      abstraction = std::move(other.abstraction);
      break;
    case Tag::union_type:
      union_type = std::move(other.union_type);
      break;
    case Tag::tuple:
      tuple = std::move(other.tuple);
      break;
  }
}

DebugType::~DebugType() {
  switch (tag) {
    case Tag::null:
      break;
    case Tag::variable:
      variable.~Variable();
      break;
    case Tag::scalar:
      scalar.~Scalar();
      break;
    case Tag::abstraction:
      abstraction.~Abstraction();
      break;
    case Tag::union_type:
      union_type.~Union();
      break;
    case Tag::tuple:
      tuple.~Tuple();
      break;
  }
}

DebugType& DebugType::operator=(const DebugType& other) {
  conditional_default_construct(other.tag);
  copy_assign(other);
  return *this;
}

DebugType& DebugType::operator=(DebugType&& other) noexcept {
  conditional_default_construct(other.tag);
  move_assign(std::move(other));
  return *this;
}

void DebugType::conditional_default_construct(DebugType::Tag other_type) noexcept {
  if (tag != other_type) {
    this->~DebugType();
    tag = other_type;
    default_construct();
  }
}

}