#pragma once

#include "mt/mt.hpp"

namespace mt {

class TypeVisitor : public TypePreservingVisitor {
public:
  explicit TypeVisitor(const Store* store) : store(store) {
    //
  }

  void root_block(const RootBlock& block) override {
    block.block->accept_const(*this);
  }

  void block(const Block& block) override {
    for (const auto& node : block.nodes) {
      node->accept_const(*this);
    }
  }

  void anonymous_function_expr(const AnonymousFunctionExpr& expr) override {
    std::cout << "X" << std::endl;
  }

  void expr_stmt(const ExprStmt& stmt) override {
    stmt.expr->accept_const(*this);
  }

  void assignment_stmt(const AssignmentStmt& stmt) override {
    stmt.of_expr->accept_const(*this);
    stmt.to_expr->accept_const(*this);
  }

  void function_def_node(const FunctionDefNode& node) override {
    Store::ReadConst reader(*store);
    const auto& def = reader.at(node.def_handle);
    if (def.body) {
      def.body->accept_const(*this);
    }
  }

private:
  const Store* store;
};

}