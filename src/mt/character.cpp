#include "character.hpp"

namespace mt {

CharacterIterator::CharacterIterator() : CharacterIterator(nullptr, 0) {
  //
}

CharacterIterator::CharacterIterator(const char* str, int64_t len) :
  current_index(0), end(len), last_character_size(0), str(str) {
  //
}

int64_t CharacterIterator::next_index() const {
  return current_index;
}

const char* CharacterIterator::data() const {
  return str;
}

int64_t CharacterIterator::size() const {
  return end;
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
  last_character_size = n_next;

  return result;
}

void CharacterIterator::advance(int64_t num) {
  for (int i = 0; i < num; i++) {
    advance();
  }
}

Character CharacterIterator::peek_previous() const {
  if (current_index == 0) {
    return Character('\0');
  } else {
    return Character(str + current_index - last_character_size, last_character_size);
  }
}

Character CharacterIterator::peek_next() const {
  return peek_nth(1);
}

Character CharacterIterator::peek_nth(int64_t num) const {
  int64_t ind = current_index;
  auto null = Character('\0');
  int num_next = 0;

  for (int64_t i = 0; i <= num; i++) {
    if (ind >= end) {
      return null;
    }

    num_next = utf8::count_code_units(str + ind, end - ind);

    if (num_next == 0) {
      return null;
    }

    if (i < num) {
      ind += num_next;
    }
  }

  if (num_next == 0) {
    return null;
  }

  return Character(str + ind, num_next);
}

Character CharacterIterator::peek() const {
  if (current_index >= end) {
    return Character('\0');
  }

  int num_next = utf8::count_code_units(str + current_index, end - current_index);

  if (num_next == 0) {
    return Character('\0');
  }

  return Character(str + current_index, num_next);
}

}