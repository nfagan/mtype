#include "stmt.hpp"
#include "StringVisitor.hpp"

namespace mt {

std::string AssignmentStmt::accept(const StringVisitor& vis) const {
  return vis.assignment_stmt(*this);
}

std::string ExprStmt::accept(const StringVisitor& vis) const {
  return vis.expr_stmt(*this);
}

}