#include "stmt.hpp"
#include "StringVisitor.hpp"
#include "../identifier_classification.hpp"
#include "visitor.hpp"

namespace mt {

/*
 * VariableDeclarationStmt
 */

void VariableDeclarationStmt::accept(TypePreservingVisitor& vis) {
  vis.variable_declaration_stmt(*this);
}

void VariableDeclarationStmt::accept_const(TypePreservingVisitor& vis) const {
  vis.variable_declaration_stmt(*this);
}

std::string VariableDeclarationStmt::accept(const StringVisitor& vis) const {
  return vis.variable_declaration_stmt(*this);
}

VariableDeclarationStmt* VariableDeclarationStmt::accept(IdentifierClassifier& classifier) {
  return classifier.variable_declaration_stmt(*this);
}

/*
 * CommandStmt
 */

void CommandStmt::accept(TypePreservingVisitor& vis) {
  vis.command_stmt(*this);
}

void CommandStmt::accept_const(TypePreservingVisitor& vis) const {
  vis.command_stmt(*this);
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

/*
 * TryStmt
 */

void TryStmt::accept(TypePreservingVisitor& vis) {
  vis.try_stmt(*this);
}

void TryStmt::accept_const(TypePreservingVisitor& vis) const {
  vis.try_stmt(*this);
}

std::string TryStmt::accept(const StringVisitor& vis) const {
  return vis.try_stmt(*this);
}

TryStmt* TryStmt::accept(IdentifierClassifier& classifier) {
  return classifier.try_stmt(*this);
}

/*
 * SwitchStmt
 */

void SwitchStmt::accept(TypePreservingVisitor& vis) {
  vis.switch_stmt(*this);
}

void SwitchStmt::accept_const(TypePreservingVisitor& vis) const {
  vis.switch_stmt(*this);
}

std::string SwitchStmt::accept(const StringVisitor& vis) const {
  return vis.switch_stmt(*this);
}

SwitchStmt* SwitchStmt::accept(IdentifierClassifier& classifier) {
  return classifier.switch_stmt(*this);
}

/*
 * WhileStmt
 */

void WhileStmt::accept(TypePreservingVisitor& vis) {
  vis.while_stmt(*this);
}

void WhileStmt::accept_const(TypePreservingVisitor& vis) const {
  vis.while_stmt(*this);
}

std::string WhileStmt::accept(const StringVisitor& vis) const {
  return vis.while_stmt(*this);
}

WhileStmt* WhileStmt::accept(IdentifierClassifier& classifier) {
  return classifier.while_stmt(*this);
}

/*
 * ControlStmt
 */

void ControlStmt::accept(TypePreservingVisitor& vis) {
  vis.control_stmt(*this);
}

void ControlStmt::accept_const(TypePreservingVisitor& vis) const {
  vis.control_stmt(*this);
}

std::string ControlStmt::accept(const StringVisitor& vis) const {
  return vis.control_stmt(*this);
}

/*
 * ForStmt
 */

void ForStmt::accept(TypePreservingVisitor& vis) {
  vis.for_stmt(*this);
}

void ForStmt::accept_const(TypePreservingVisitor& vis) const {
  vis.for_stmt(*this);
}

std::string ForStmt::accept(const StringVisitor& vis) const {
  return vis.for_stmt(*this);
}

ForStmt* ForStmt::accept(IdentifierClassifier& classifier) {
  return classifier.for_stmt(*this);
}

/*
 * IfStmt
 */

void IfStmt::accept(TypePreservingVisitor& vis) {
  vis.if_stmt(*this);
}

void IfStmt::accept_const(TypePreservingVisitor& vis) const {
  vis.if_stmt(*this);
}

std::string IfStmt::accept(const StringVisitor& vis) const {
  return vis.if_stmt(*this);
}

IfStmt* IfStmt::accept(IdentifierClassifier& classifier) {
  return classifier.if_stmt(*this);
}

/*
 * AssignmentStmt
 */

void AssignmentStmt::accept(TypePreservingVisitor& vis) {
  vis.assignment_stmt(*this);
}

void AssignmentStmt::accept_const(TypePreservingVisitor& vis) const {
  vis.assignment_stmt(*this);
}

std::string AssignmentStmt::accept(const StringVisitor& vis) const {
  return vis.assignment_stmt(*this);
}

AssignmentStmt* AssignmentStmt::accept(IdentifierClassifier& classifier) {
  return classifier.assignment_stmt(*this);
}

/*
 * ExprStmt
 */

void ExprStmt::accept(TypePreservingVisitor& vis) {
  vis.expr_stmt(*this);
}

void ExprStmt::accept_const(TypePreservingVisitor& vis) const {
  vis.expr_stmt(*this);
}

std::string ExprStmt::accept(const StringVisitor& vis) const {
  return vis.expr_stmt(*this);
}

ExprStmt* ExprStmt::accept(IdentifierClassifier& classifier) {
  return classifier.expr_stmt(*this);
}

}