#include "traversal.hpp"
#include "ast.hpp"
#include <cassert>

namespace mt {

/*
* ClassDefState
*/

void ClassDefState::push_class(const mt::ClassDefHandle& handle) {
  class_defs.push_back(handle);
}

void ClassDefState::pop_class() {
  assert(!class_defs.empty() && "No class to pop.");
  class_defs.pop_back();
}

ClassDefHandle ClassDefState::enclosing_class() const {
  return class_defs.empty() ? ClassDefHandle() : class_defs.back();
}

void ClassDefState::push_default_state() {
  push_non_superclass_method_application();
}

void ClassDefState::push_superclass_method_application() {
  is_superclass_method_application.push_back(true);
}

void ClassDefState::push_non_superclass_method_application() {
  is_superclass_method_application.push_back(false);
}

void ClassDefState::pop_superclass_method_application() {
  assert(!is_superclass_method_application.empty() && "No superclass method application side to pop.");
  is_superclass_method_application.pop_back();
}

bool ClassDefState::is_within_superclass_method_application() const {
  return is_superclass_method_application.back();
}

/*
 * FunctionDefState
 */

FunctionDefHandle FunctionDefState::enclosing_function() const {
  return function_defs.empty() ? FunctionDefHandle() : function_defs.back();
}

void FunctionDefState::push_function(const mt::FunctionDefHandle& func) {
  function_defs.push_back(func);
}

void FunctionDefState::pop_function() {
  assert(!function_defs.empty() && "No function to pop.");
  function_defs.pop_back();
}

}
