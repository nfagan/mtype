#include "character.hpp"

namespace mt {

CharacterIterator::CharacterIterator() : CharacterIterator(nullptr, 0) {
  //
}

CharacterIterator::CharacterIterator(const char* str, int64_t len) :
  current_index(0), end(len), str(str) {
  //
}

int64_t CharacterIterator::next_index() const {
  return current_index;
}

const char* CharacterIterator::data() const {
  return str;
}

bool CharacterIterator::has_next() const {
  return current_index < end;
}

Character CharacterIterator::advance() {
  int n_next = utf8::count_code_units(str + current_index, end - current_index);

  //  Invalid character.
  if (n_next == 0) {
    current_index = end;

    return Character('\0');
  }

  Character result(str + current_index, n_next);
  current_index += n_next;

  return result;
}

Character CharacterIterator::peek() const {
  if (current_index >= end) {
    return Character('\0');
  }

  int n_next = utf8::count_code_units(str + current_index, end - current_index);

  if (n_next == 0) {
    return Character('\0');
  }

  return Character(str + current_index, n_next);
}

}