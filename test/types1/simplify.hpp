#pragma once

namespace mt {

class Unifier;

class Simplifier {
public:
  Simplifier(Unifier& unifier) : unifier(unifier) {
    //
  }

private:
  Unifier& unifier;
};

}