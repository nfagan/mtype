#include "def.hpp"
#include "StringVisitor.hpp"

namespace mt {

std::string Block::accept(const StringVisitor& vis) const {
  return vis.block(*this);
}

std::string FunctionDef::accept(const StringVisitor& vis) const {
  return vis.function_def(*this);
}

}