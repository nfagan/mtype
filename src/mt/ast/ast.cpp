#include "ast.hpp"
#include "StringVisitor.hpp"
#include "visitor.hpp"
#include "../identifier_classification.hpp"

namespace mt {

/*
 * RootBlock
 */

void RootBlock::accept(TypePreservingVisitor& vis) {
  vis.root_block(*this);
}

void RootBlock::accept_const(TypePreservingVisitor& vis) const {
  vis.root_block(*this);
}

RootBlock* RootBlock::accept(IdentifierClassifier& classifier) {
  return classifier.root_block(*this);
}

std::string RootBlock::accept(const StringVisitor& vis) const {
  return vis.root_block(*this);
}

/*
 * Block
 */

void Block::accept(TypePreservingVisitor& vis) {
  vis.block(*this);
}

void Block::accept_const(TypePreservingVisitor& vis) const {
  vis.block(*this);
}

std::string Block::accept(const StringVisitor& vis) const {
  return vis.block(*this);
}

Block* Block::accept(IdentifierClassifier& classifier) {
  return classifier.block(*this);
}

}