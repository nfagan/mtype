#include "type_annot.hpp"
#include "StringVisitor.hpp"

namespace mt {

std::string TypeAnnotMacro::accept(const StringVisitor& vis) const {
  return vis.type_annot_macro(*this);
}

std::string InlineType::accept(const StringVisitor& vis) const {
  return vis.inline_type(*this);
}

std::string TypeGiven::accept(const StringVisitor& vis) const {
  return vis.type_given(*this);
}

std::string TypeLet::accept(const StringVisitor& vis) const {
  return vis.type_let(*this);
}

std::string TypeBegin::accept(const StringVisitor& vis) const {
  return vis.type_begin(*this);
}

std::string ScalarType::accept(const StringVisitor& vis) const {
  return vis.scalar_type(*this);
}

std::string FunctionType::accept(const StringVisitor& vis) const {
  return vis.function_type(*this);
}

}
