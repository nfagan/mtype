#include "def.hpp"
#include "StringVisitor.hpp"
#include "../identifier_classification.hpp"

namespace mt {

ClassDefReference* ClassDefReference::accept(IdentifierClassifier& classifier) {
  return classifier.class_def_reference(*this);
}

std::string ClassDefReference::accept(const StringVisitor& vis) const {
  return vis.class_def_reference(*this);
}

std::string ClassDef::accept(const StringVisitor& vis) const {
  return vis.class_def(*this);
}

std::string FunctionDef::accept(const StringVisitor& vis) const {
  return vis.function_def(*this);
}

std::string VariableDef::accept(const mt::StringVisitor& vis) const {
  return vis.variable_def(*this);
}

}