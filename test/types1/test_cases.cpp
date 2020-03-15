#include "test_cases.hpp"
#include "type_store.hpp"
#include "type_equality.hpp"
#include <cassert>

#define MT_SHOW_ERROR(msg) \
  std::cout << "FAIL: " << msg << std::endl

namespace mt {

void test_equivalence() {
  auto t0 = std::chrono::high_resolution_clock::now();

  using Use = types::DestructuredTuple::Usage;

  StringRegistry str_registry;
  TypeStore store;
  store.make_builtin_types();
  TypeEquality equiv(store, str_registry);
  TypeEquality::TypeEquivalenceComparator eq(equiv);

  const auto& d_handle = store.double_type_handle;
  const auto& c_handle = store.char_type_handle;

  auto tup = store.make_destructured_tuple(Use::rvalue, d_handle);
  auto rec_tup = store.make_destructured_tuple(Use::rvalue, tup);
  auto mult_double_tup = store.make_destructured_tuple(Use::rvalue, d_handle, d_handle);
  auto mult_rec_double_tup = store.make_destructured_tuple(Use::rvalue, std::vector<TypeHandle>{d_handle, d_handle, rec_tup});
  auto list_double = store.make_list(types::List::Usage::definition, d_handle);
  auto tup_list_double_rvalue = store.make_destructured_tuple(Use::rvalue, list_double);
  auto tup_list_double_lvalue = store.make_destructured_tuple(Use::lvalue, list_double);

  auto list_pattern = store.make_list(types::List::Usage::definition, std::vector<TypeHandle>{d_handle, d_handle, c_handle});
  auto tup_list_pattern = store.make_destructured_tuple(Use::rvalue, list_pattern);
  auto tup_match_pattern = store.make_destructured_tuple(Use::lvalue, std::vector<TypeHandle>{d_handle, d_handle, c_handle});
  auto tup_wrong_pattern = store.make_destructured_tuple(Use::lvalue, std::vector<TypeHandle>{d_handle, d_handle, c_handle, d_handle});

  auto nest_tup_pattern = store.make_destructured_tuple(Use::lvalue, std::vector<TypeHandle>{tup_match_pattern, tup_match_pattern});

  if (!eq.equivalence(tup, rec_tup)) {
    MT_SHOW_ERROR("Failed to match r value tuples.");
  }
  if (!eq.equivalence(rec_tup, tup)) {
    MT_SHOW_ERROR("Failed to match r value tuples; ordering effects.");
  }
  if (!eq.equivalence(rec_tup, store.double_type_handle)) {
    MT_SHOW_ERROR("Failed to unroll destructured tuple.");
  }
  if (!eq.equivalence(store.double_type_handle, store.double_type_handle)) {
    MT_SHOW_ERROR("Failed to match double to double.");
  }
  if (eq.equivalence(tup, store.char_type_handle)) {
    MT_SHOW_ERROR("Matched double tup with char.");
  }
  if (eq.equivalence(tup, mult_double_tup)) {
    MT_SHOW_ERROR("Matched double tup with multi-double tup.");
  }
  if (eq.equivalence(tup_list_double_rvalue, mult_double_tup)) {
    MT_SHOW_ERROR("Matched multi-double rvalue tup with rvalue double list.");
  }
  if (!eq.equivalence(tup_list_double_lvalue, mult_double_tup)) {
    MT_SHOW_ERROR("Failed to match l-value tuple list of double with rvalue mult double tuple.");
  }
  if (!eq.equivalence(mult_double_tup, tup_list_double_lvalue)) {
    MT_SHOW_ERROR("Failed to match l-value tuple list of double with rvalue mult double tuple; ordering effects.");
  }
  if (!eq.equivalence(mult_rec_double_tup, tup_list_double_lvalue)) {
    MT_SHOW_ERROR("Failed to match l-value tuple list of double with rvalue mult double tuple.");
  }
  if (!eq.equivalence(list_pattern, list_pattern)) {
    MT_SHOW_ERROR("Failed to match same lists.");
  }
  if (!eq.equivalence(tup_list_pattern, tup_match_pattern)) {
    MT_SHOW_ERROR("Failed to match multi-element list pattern to destructured tuple.");
  }
  if (eq.equivalence(tup_list_pattern, tup_wrong_pattern)) {
    MT_SHOW_ERROR("Matched destructured tuple with wrong pattern to patterned list.");
  }
  if (!eq.equivalence(nest_tup_pattern, tup_list_pattern)) {
    MT_SHOW_ERROR("Nested tuple of pattern failed to match pattern.");
  }

  auto t1 = std::chrono::high_resolution_clock::now();
  std::cout << "Test time: " << (std::chrono::duration<double>(t1-t0).count() * 1e3) << " (ms)" << std::endl;
}

void run_all(const TypeVisitor& visitor) {
  test_equivalence();
}

#undef MT_SHOW_ERROR

}
