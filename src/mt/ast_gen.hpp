#pragma once

#include "ast.hpp"
#include "token.hpp"

namespace mt {
class AstGenerator {
public:
  AstGenerator() = default;
  ~AstGenerator() = default;

  void parse(const std::vector<Token>& tokens);


private:
  TokenIterator iterator;
};
}