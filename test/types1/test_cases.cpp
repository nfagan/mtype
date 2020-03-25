#include "test_cases.hpp"
#include "type_store.hpp"
#include "type_relation.hpp"
#include "type_relationships.hpp"
#include "debug.hpp"
#include "library.hpp"
#include <cassert>

#define MT_SHOW_ERROR(msg) \
  std::cout << "FAIL: " << msg << std::endl

#define MT_SHOW_ERROR_PRINT2(msg, a, b) \
  std::cout << "FAIL: " << msg << std::endl; \
  DebugTypePrinter(store, &str_registry).show2((a), (b));

#define MT_ERROR_IF_NON_EQUIV(msg, a, b) \
  if (!eq.related_entry((a), (b))) { \
    std::cout << "FAIL: " << msg << std::endl; \
    DebugTypePrinter(store, &str_registry).show2((a), (b)); \
  }

#define MT_ERROR_IF_EQUIV(msg, a, b) \
  if (eq.related_entry((a), (b))) { \
    std::cout << "FAIL: " << msg << std::endl; \
    DebugTypePrinter(store, &str_registry).show2((a), (b)); \
  }

#define MT_ERROR_IF_RELATED(relation, msg, a, b) \
  if (relation.related_entry((a), (b))) { \
    std::cout << "FAIL (expected no relation): " << msg << std::endl; \
    DebugTypePrinter(store, &str_registry).show2((a), (b)); \
  }

#define MT_ERROR_IF_UNRELATED(relation, msg, a, b) \
  if (!relation.related_entry((a), (b))) { \
    std::cout << "FAIL (expected relation): " << msg << std::endl; \
    DebugTypePrinter(store, &str_registry).show2((a), (b)); \
  }

namespace mt {

void test_subtyping() {
  using Use = types::DestructuredTuple::Usage;

  StringRegistry str_registry;
  TypeStore store;
  Library library(store, str_registry);
  SubtypeRelation subtype_relation(library);
  library.make_known_types();
  TypeRelation relation(subtype_relation, store);
  TypeRelation::ArgumentComparator arg_compare(relation);

  const auto& d_handle = library.double_type_handle;
  const auto& sub_d_handle = library.sub_double_type_handle;

  const auto sum_double_args = store.make_input_destructured_tuple(d_handle);
  const auto sum_double_outs = store.make_output_destructured_tuple(d_handle);
  const auto sum_name = MatlabIdentifier(str_registry.register_string("sum"));
  const auto sum_double = store.make_abstraction(sum_name, sum_double_args, sum_double_outs);
  const auto sum_sub_double_args = store.make_input_destructured_tuple(sub_d_handle);
  const auto sum_sub_double_outs = store.make_output_destructured_tuple(sub_d_handle);
  const auto sum_sub_double = store.make_abstraction(sum_name, sum_sub_double_args, sum_sub_double_outs);
  const auto sum_subtype = store.make_abstraction(sum_name, sum_double_args, sum_sub_double_outs);

  const auto nested_tup_double = store.make_rvalue_destructured_tuple(sum_double_outs);
  const auto nested_tup_sub_double = store.make_rvalue_destructured_tuple(sum_sub_double_outs);

  const auto list_subtype = store.make_list(sub_d_handle);
  const auto list_type = store.make_list(d_handle);

  const auto mult_double = store.make_rvalue_destructured_tuple(TypeHandles{d_handle, d_handle});
  const auto wrap_list = store.make_rvalue_destructured_tuple(list_type);

  MT_ERROR_IF_UNRELATED(relation, "", sub_d_handle, d_handle)
  MT_ERROR_IF_RELATED(relation, "", d_handle, sub_d_handle)
  MT_ERROR_IF_RELATED(relation, "", sum_sub_double, sum_double)
  MT_ERROR_IF_RELATED(relation, "", sum_double, sum_sub_double)
  MT_ERROR_IF_UNRELATED(relation, "", sum_subtype, sum_double)
  MT_ERROR_IF_RELATED(relation, "", sum_double, sum_subtype)
  MT_ERROR_IF_UNRELATED(relation, "", list_subtype, list_type)
  MT_ERROR_IF_RELATED(relation, "", list_type, list_subtype)
  MT_ERROR_IF_UNRELATED(relation, "", nested_tup_sub_double, nested_tup_double)
  MT_ERROR_IF_UNRELATED(relation, "", nested_tup_sub_double, nested_tup_sub_double)
  MT_ERROR_IF_RELATED(relation, "", nested_tup_double, nested_tup_sub_double)
  MT_ERROR_IF_UNRELATED(relation, "", list_subtype, d_handle)
  MT_ERROR_IF_RELATED(relation, "", d_handle, list_subtype)
  MT_ERROR_IF_UNRELATED(relation, "", list_subtype, wrap_list)
  MT_ERROR_IF_RELATED(relation, "", wrap_list, list_subtype)
  MT_ERROR_IF_UNRELATED(relation, "", wrap_list, mult_double)
  MT_ERROR_IF_UNRELATED(relation, "", sum_sub_double_args, nested_tup_double)
  MT_ERROR_IF_UNRELATED(relation, "", nested_tup_sub_double, sum_double_args)
}

void test_equivalence_debug() {
  using Use = types::DestructuredTuple::Usage;

  StringRegistry str_registry;
  TypeStore store;
  EquivalenceRelation equiv;
  TypeRelation eq(equiv, store);
  Library library(store, str_registry);
  library.make_known_types();

  const auto& d_handle = library.double_type_handle;
  const auto& c_handle = library.char_type_handle;

  auto tup = store.make_destructured_tuple(Use::rvalue, TypeHandles{d_handle, d_handle});
  auto tup2 = store.make_destructured_tuple(Use::rvalue, TypeHandles{d_handle, c_handle});
  auto tup3 = store.make_destructured_tuple(Use::rvalue, TypeHandles{c_handle});
  auto tup4 = store.make_destructured_tuple(Use::rvalue, TypeHandles{d_handle});
  auto rec_tup = store.make_destructured_tuple(Use::rvalue, tup);
  auto rec_tup2 = store.make_destructured_tuple(Use::rvalue, tup2);
  auto rec_tup3 = store.make_destructured_tuple(Use::rvalue, tup3);
  auto rec_tup4 = store.make_destructured_tuple(Use::rvalue, tup4);
  auto empty_tup = store.make_destructured_tuple(Use::rvalue, TypeHandles{});
  auto out_tup = store.make_destructured_tuple(Use::definition_outputs, TypeHandles{c_handle, d_handle});
  auto out_tup2 = store.make_destructured_tuple(Use::definition_outputs, TypeHandles{d_handle, c_handle});
  auto wrap_tup = store.make_destructured_tuple(Use::rvalue, TypeHandles{out_tup});

  auto wrap_out_tup = store.make_destructured_tuple(Use::rvalue, TypeHandles{out_tup, c_handle});
  auto match_in_tup = store.make_destructured_tuple(Use::definition_inputs, TypeHandles{c_handle, c_handle});

  auto mixed_wrap_tup = store.make_destructured_tuple(Use::rvalue, TypeHandles{tup, d_handle, tup2});
  auto flat_mixed_tup = store.make_destructured_tuple(Use::rvalue, TypeHandles{d_handle, d_handle, d_handle, d_handle, c_handle});

  auto list1 = store.make_list(TypeHandles{c_handle, d_handle});
  auto tup_list1 = store.make_destructured_tuple(Use::rvalue, TypeHandles{list1});
  auto tup_match_list1 = store.make_destructured_tuple(Use::rvalue, TypeHandles{c_handle, d_handle});
  auto rec_tup_match_list1 = store.make_destructured_tuple(Use::rvalue, TypeHandles{tup_match_list1, tup_match_list1});
  auto rec_tup_match_list2 = store.make_destructured_tuple(Use::rvalue, TypeHandles{c_handle, rec_tup4, c_handle, d_handle});

  auto input_tup = store.make_destructured_tuple(Use::definition_inputs, TypeHandles{c_handle, d_handle});
  auto match_input_tup = store.make_destructured_tuple(Use::rvalue, TypeHandles{tup3, tup4});

  auto flat_l_tup = store.make_destructured_tuple(Use::lvalue, TypeHandles{c_handle});
  auto wrap_l_tup = store.make_destructured_tuple(Use::lvalue, TypeHandles{flat_l_tup});

  auto mixed_rec_tup = store.make_destructured_tuple(Use::rvalue, TypeHandles{d_handle, rec_tup, d_handle});
  auto flat_tup = store.make_destructured_tuple(Use::rvalue, TypeHandles{d_handle, d_handle, d_handle, d_handle});

  // list[dt-r[dt-r[s0]], s0, dt-o[s0, s2]]
  auto mixed_list1 = store.make_list(TypeHandles{rec_tup4, d_handle, out_tup2});
  auto test_list1 = store.make_list(TypeHandles{d_handle, d_handle, d_handle});
  auto wrap_list1 = store.make_destructured_tuple(Use::definition_inputs, test_list1);
  auto wrap_list2 = store.make_destructured_tuple(Use::rvalue, mixed_list1);

  auto list_double = store.make_list(d_handle);
  auto out1 = store.make_destructured_tuple(Use::definition_outputs, store.make_list(d_handle));
  auto wrap_out1 = store.make_destructured_tuple(Use::rvalue, out1);
  auto wrap_in_list = store.make_destructured_tuple(Use::definition_inputs, list_double);

  auto list_tup_double_char = store.make_list(tup2);
  auto list_double_char = store.make_list(TypeHandles{d_handle, c_handle});

  if (!eq.related_entry(mixed_wrap_tup, flat_mixed_tup)) {
    MT_SHOW_ERROR("Flattened type sequence not equal equivalent nested sequence.");
  }
  if (!eq.related_entry(rec_tup4, d_handle)) {
    MT_SHOW_ERROR("Double handle not equal to recursive scalar tuple.");
  }
  if (eq.related_entry(rec_tup4, c_handle)) {
    MT_SHOW_ERROR("Recursive recursive scalar double tuple equal to char.");
  }
  if (!eq.related_entry(rec_tup, rec_tup)) {
    MT_SHOW_ERROR("Recursive tuples were not equivalent.");
  }
  if (eq.related_entry(rec_tup, rec_tup2)) {
    MT_SHOW_ERROR("Non equiv double-char tuples were marked equivalent.");
  }
  if (eq.related_entry(rec_tup2, rec_tup3)) {
    MT_SHOW_ERROR("Non equiv char tuples were marked equivalent.");
  }
  if (eq.related_entry(rec_tup, rec_tup4)) {
    MT_SHOW_ERROR("Non equiv double tuples were marked equivalent.");
  }
  if (!eq.related_entry(empty_tup, empty_tup)) {
    MT_SHOW_ERROR("Empty tuples were not equiv.");
  }
  if (eq.related_entry(empty_tup, tup2)) {
    MT_SHOW_ERROR("Tuples were equiv when 1 empty.");
  }
  if (!eq.related_entry(mixed_rec_tup, flat_tup)) {
    MT_SHOW_ERROR("Mixed flat and recursive double tups were not equiv.");
  }

  MT_ERROR_IF_EQUIV("Recursive tuple wrapping char outputs equal to char.", wrap_tup, c_handle)

  if (!eq.related_entry(wrap_tup, tup3)) {
    MT_SHOW_ERROR("Recursive tuple wrapping char outputs not equal to rvalue tuple wrapping char.");
  }
  if (!eq.related_entry(flat_l_tup, out_tup)) {
    MT_SHOW_ERROR("Flat l-value char tuple not equal to flat [char, double] outputs.");
  }
  if (!eq.related_entry(wrap_l_tup, out_tup)) {
    MT_SHOW_ERROR("Wrapped l-value char tuple not equal to flat [char, double] outputs.");
  }
  if (!eq.related_entry(rec_tup_match_list1, tup_list1)) {
    MT_SHOW_ERROR("Failed to match tuple of list of matching types with tuple of list.");
  }
  if (!eq.related_entry(rec_tup_match_list2, tup_list1)) {
    MT_SHOW_ERROR("Failed to match tuple of list of matching types with tuple of list.");
  }
  if (!eq.related_entry(tup_list1, rec_tup_match_list2)) {
    MT_SHOW_ERROR("Failed to match tuple of list of matching types with tuple of list.");
  }
  if (!eq.related_entry(tup_match_list1, tup_list1)) {
    MT_SHOW_ERROR("Failed to match tuple of list of matching types with tuple of list.");
  }
  if (eq.related_entry(rec_tup, tup_list1)) {
    MT_SHOW_ERROR("Matched incorrect tuple with tuple list.");
  }
  if (eq.related_entry(input_tup, tup_list1)) {
    MT_SHOW_ERROR("Matched input tuple with list 1.");
  }
  if (!eq.related_entry(input_tup, match_input_tup)) {
    MT_SHOW_ERROR("Failed to match input tuple with nested equiv r value tuple.");
  }
  if (!eq.related_entry(match_in_tup, wrap_out_tup)) {
    MT_SHOW_ERROR_PRINT2("Failed to match: ", match_in_tup, wrap_out_tup)
  }
  if (!eq.related_entry(mixed_list1, test_list1)) {
    MT_SHOW_ERROR_PRINT2("Failed to match: ", mixed_list1, test_list1)
  }
  if (!eq.related_entry(wrap_list1, wrap_list2)) {
    MT_SHOW_ERROR_PRINT2("Failed to match: ", wrap_list1, wrap_list2)
  }
  if (!eq.related_entry(wrap_out1, wrap_in_list)) {
    MT_SHOW_ERROR_PRINT2("Failed to match: ", wrap_out1, wrap_in_list)
  }

  MT_ERROR_IF_UNRELATED(eq, "", list_tup_double_char, list_double_char)
}

void test_equivalence() {
  auto t0 = std::chrono::high_resolution_clock::now();

  using Use = types::DestructuredTuple::Usage;

  StringRegistry str_registry;
  TypeStore store;
  EquivalenceRelation equiv;
  TypeRelation eq(equiv, store);
  Library library(store, str_registry);
  library.make_known_types();

  const auto& d_handle = library.double_type_handle;
  const auto& c_handle = library.char_type_handle;

  auto tup = store.make_destructured_tuple(Use::rvalue, d_handle);
  auto rec_tup = store.make_destructured_tuple(Use::rvalue, tup);
  auto mult_double_tup = store.make_destructured_tuple(Use::rvalue, d_handle, d_handle);
  auto mult_rec_double_tup = store.make_destructured_tuple(Use::rvalue, TypeHandles{d_handle, d_handle, rec_tup});
  auto list_double = store.make_list(d_handle);
  auto tup_list_double_rvalue = store.make_destructured_tuple(Use::rvalue, list_double);
  auto tup_list_double_lvalue = store.make_destructured_tuple(Use::lvalue, list_double);

  auto list_pattern = store.make_list(TypeHandles{d_handle, d_handle, c_handle});
  auto tup_list_pattern = store.make_destructured_tuple(Use::rvalue, list_pattern);
  auto tup_match_pattern = store.make_destructured_tuple(Use::lvalue, TypeHandles{d_handle, d_handle, c_handle});
  auto tup_wrong_pattern = store.make_destructured_tuple(Use::lvalue, TypeHandles{d_handle, d_handle, c_handle, d_handle});

  auto nest_tup_pattern = store.make_destructured_tuple(Use::lvalue, TypeHandles{tup_match_pattern, tup_match_pattern});

  if (!eq.related_entry(tup, rec_tup)) {
    MT_SHOW_ERROR("Failed to match r value tuples.");
  }
  if (!eq.related_entry(rec_tup, tup)) {
    MT_SHOW_ERROR("Failed to match r value tuples; ordering effects.");
  }
  if (!eq.related_entry(rec_tup, d_handle)) {
    MT_SHOW_ERROR("Failed to unroll destructured tuple.");
  }
  if (!eq.related_entry(d_handle, d_handle)) {
    MT_SHOW_ERROR("Failed to match double to double.");
  }
  if (eq.related_entry(tup, c_handle)) {
    MT_SHOW_ERROR("Matched double tup with char.");
  }
  if (eq.related_entry(tup, mult_double_tup)) {
    MT_SHOW_ERROR("Matched double tup with multi-double tup.");
  }
  if (!eq.related_entry(tup_list_double_rvalue, mult_double_tup)) {
    MT_SHOW_ERROR_PRINT2("Failed to match: ", tup_list_double_rvalue, mult_double_tup)
  }
  if (!eq.related_entry(tup_list_double_lvalue, mult_double_tup)) {
    MT_SHOW_ERROR("Failed to match l-value tuple list of double with rvalue mult double tuple.");
  }
  if (!eq.related_entry(mult_double_tup, tup_list_double_lvalue)) {
    MT_SHOW_ERROR("Failed to match l-value tuple list of double with rvalue mult double tuple; ordering effects.");
  }
  if (!eq.related_entry(mult_rec_double_tup, tup_list_double_lvalue)) {
    MT_SHOW_ERROR("Failed to match l-value tuple list of double with rvalue mult double tuple.");
  }
  if (!eq.related_entry(list_pattern, list_pattern)) {
    MT_SHOW_ERROR("Failed to match same lists.");
  }
  if (!eq.related_entry(tup_list_pattern, tup_match_pattern)) {
    MT_SHOW_ERROR("Failed to match multi-element list pattern to destructured tuple.");
  }
  if (eq.related_entry(tup_list_pattern, tup_wrong_pattern)) {
    MT_SHOW_ERROR("Matched destructured tuple with wrong pattern to patterned list.");
  }
  if (!eq.related_entry(nest_tup_pattern, tup_list_pattern)) {
    MT_SHOW_ERROR("Nested tuple of pattern failed to match pattern.");
  }

  auto t1 = std::chrono::high_resolution_clock::now();
  std::cout << "Test time: " << (std::chrono::duration<double>(t1-t0).count() * 1e3) << " (ms)" << std::endl;
}

void run_all() {
  test_equivalence();
  test_equivalence_debug();
  test_subtyping();
}

#undef MT_SHOW_ERROR

}
