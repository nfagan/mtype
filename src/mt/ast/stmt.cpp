#include "stmt.hpp"
#include "StringVisitor.hpp"

namespace mt {

std::string ForStmt::accept(const StringVisitor& vis) const {
  return vis.for_stmt(*this);
}

std::string IfStmt::accept(const StringVisitor& vis) const {
  return vis.if_stmt(*this);
}

std::string IfBranch::accept(const StringVisitor& vis) const {
  return vis.if_branch(*this, "if");
}

std::string ElseBranch::accept(const StringVisitor& vis) const {
  return vis.else_branch(*this);
}

std::string AssignmentStmt::accept(const StringVisitor& vis) const {
  return vis.assignment_stmt(*this);
}

std::string ExprStmt::accept(const StringVisitor& vis) const {
  return vis.expr_stmt(*this);
}

}