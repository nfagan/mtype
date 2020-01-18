#include <iostream>
#include <mt/scan.hpp>
#include "mt/string.hpp"
#include "mt/character.hpp"

namespace {
void test_string(const std::string& str, const mt::Character& delim, int expect_num) {
  auto res = mt::split(str, delim);

  if (res.size() != expect_num) {
    std::cout << "Expected " << expect_num << " elements in split." << std::endl;
    return;
  }

  for (const auto& s : res) {
    std::cout << "Split by " << delim << "; was: " << s << std::endl;

    mt::CharacterIterator it(s.data(), s.size());

    while (it.has_next()) {
      auto split_c = it.advance();
      if (split_c == delim) {
        std::cout << "Expected no delimiter in split string." << std::endl;
        return;
      }
    }
  }

  auto joined = mt::join(res, delim);

  if (joined != str) {
    std::cout << "Expected joined string to equal split string." << std::endl;
    std::cout << "Joined was: " << joined << std::endl << "orig was: " << str << std::endl;
    std::cout << "Joined size was: " << joined.size() << std::endl;
    std::cout << "Orig size was: " << str.size() << std::endl;
  }
}

void test_split() {
  std::vector<std::string> strs;
  std::vector<int> expect_num({4, 4});
  strs.emplace_back("éeéé\néeüü\n\n");
  strs.emplace_back("\n\néeéé\née");

  mt::Character delim('\n');

  for (int i = 0; i < strs.size(); i++) {
    test_string(strs[i], delim, expect_num[i]);
  }
}

}

int main(int argc, char** argv) {
  test_split();
  return 0;
}