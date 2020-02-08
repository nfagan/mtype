#include "stmt.hpp"
#include "StringVisitor.hpp"
#include "../identifier_classification.hpp"

namespace mt {

std::string VariableDeclarationStmt::accept(const StringVisitor& vis) const {
  return vis.variable_declaration_stmt(*this);
}

VariableDeclarationStmt* VariableDeclarationStmt::accept(IdentifierClassifier& classifier) {
  return classifier.variable_declaration_stmt(*this);
}

CommandStmt::CommandStmt(const Token& source_token,
                         int64_t identifier,
                         std::vector<CharLiteralExpr>&& arguments) :
                         source_token(source_token),
                         command_identifier(identifier),
                         arguments(std::move(arguments)) {
  //
}

std::string CommandStmt::accept(const StringVisitor& vis) const {
  return vis.command_stmt(*this);
}

std::string TryStmt::accept(const StringVisitor& vis) const {
  return vis.try_stmt(*this);
}

std::string SwitchStmt::accept(const StringVisitor& vis) const {
  return vis.switch_stmt(*this);
}

SwitchStmt* SwitchStmt::accept(IdentifierClassifier& classifier) {
  return classifier.switch_stmt(*this);
}

std::string WhileStmt::accept(const StringVisitor& vis) const {
  return vis.while_stmt(*this);
}

WhileStmt* WhileStmt::accept(IdentifierClassifier& classifier) {
  return classifier.while_stmt(*this);
}

std::string ControlStmt::accept(const StringVisitor& vis) const {
  return vis.control_stmt(*this);
}

std::string ForStmt::accept(const StringVisitor& vis) const {
  return vis.for_stmt(*this);
}

ForStmt* ForStmt::accept(IdentifierClassifier& classifier) {
  return classifier.for_stmt(*this);
}

std::string IfStmt::accept(const StringVisitor& vis) const {
  return vis.if_stmt(*this);
}

IfStmt* IfStmt::accept(IdentifierClassifier& classifier) {
  return classifier.if_stmt(*this);
}

std::string AssignmentStmt::accept(const StringVisitor& vis) const {
  return vis.assignment_stmt(*this);
}

AssignmentStmt* AssignmentStmt::accept(IdentifierClassifier& classifier) {
  return classifier.assignment_stmt(*this);
}

std::string ExprStmt::accept(const StringVisitor& vis) const {
  return vis.expr_stmt(*this);
}

ExprStmt* ExprStmt::accept(IdentifierClassifier& classifier) {
  return classifier.expr_stmt(*this);
}

}