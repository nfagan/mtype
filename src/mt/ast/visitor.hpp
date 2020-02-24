#pragma once

namespace mt {

struct RootBlock;

class TypePreservingVisitor {
public:
  virtual void root_block(RootBlock&) {}
  virtual void root_block(const RootBlock&) const {}
};

}