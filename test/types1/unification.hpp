#pragma once

#include "mt/mt.hpp"
#include "type.hpp"
#include "type_equality.hpp"
#include "type_store.hpp"
#include "instance.hpp"
#include "simplify.hpp"
#include <map>

namespace mt {

class TypeVisitor;
class DebugTypePrinter;
class Library;

class Unifier {
  friend class Simplifier;
public:
  using BoundVariables = std::unordered_map<TypeHandle, TypeHandle, TypeHandle::Hash>;
public:
  Unifier(TypeStore& store, const Library& library, StringRegistry& string_registry) :
  store(store),
  library(library),
  string_registry(string_registry),
  simplifier(*this, store),
  instantiation(store) {
    //
  }

  ~Unifier() {
    std::cout << "Num type eqs: " << type_equations.size() << std::endl;
    std::cout << "Subs size: " << bound_variables.size() << std::endl;
    std::cout << "Num types: " << store.size() << std::endl;
  }

  void push_type_equation(TypeEquation&& eq) {
    type_equations.emplace_back(std::move(eq));
  }

  void unify();
  Optional<TypeHandle> bound_type(const TypeHandle& for_type) const;

private:
  using TermRef = const TypeEquationTerm&;
  using TypeRef = const TypeHandle&;

  void unify_one(TypeEquation eq);

  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term, types::Abstraction& func);
  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term, types::Variable& var);
  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term, types::Tuple& tup);
  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term, types::DestructuredTuple& tup);
  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term, types::Subscript& sub);
  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term, types::List& list);
  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term, types::Assignment& assignment);
  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term);
  void apply_to(std::vector<TypeHandle>& sources, TermRef term);

  MT_NODISCARD TypeHandle substitute_one(TypeRef source, TermRef term, TermRef lhs, TermRef rhs);
  void substitute_one(std::vector<TypeHandle>& sources, TermRef term, TermRef lhs, TermRef rhs);

  MT_NODISCARD TypeHandle substitute_one(types::Variable& var,           TypeRef source, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD TypeHandle substitute_one(types::Abstraction& func,       TypeRef source, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD TypeHandle substitute_one(types::Tuple& tup,              TypeRef source, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD TypeHandle substitute_one(types::DestructuredTuple& tup,  TypeRef source, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD TypeHandle substitute_one(types::Subscript& sub,          TypeRef source, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD TypeHandle substitute_one(types::List& list,              TypeRef source, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD TypeHandle substitute_one(types::Assignment& assignment,  TypeRef source, TermRef term, TermRef lhs, TermRef rhs);

  Type::Tag type_of(TypeRef handle) const;
  void show();

  TypeHandle maybe_unify_subscript(TypeRef source, types::Subscript& sub);
  TypeHandle maybe_unify_known_subscript_type(TypeRef source, types::Subscript& sub);
  TypeHandle maybe_unify_function_call_subscript(TypeRef source, const types::Abstraction& source_func, types::Subscript& sub);

  void check_assignment(TypeRef source, const types::Assignment& assignment);
  void check_push_func(TypeRef source, const types::Abstraction& func);

  bool is_known_subscript_type(TypeRef handle) const;
  bool is_concrete_argument(TypeRef handle) const;
  bool are_concrete_arguments(const TypeHandles& handles) const;

  void flatten_list(TypeRef source, std::vector<TypeHandle>& into) const;

  DebugTypePrinter type_printer() const;

private:
  TypeStore& store;
  const Library& library;
  StringRegistry& string_registry;
  Simplifier simplifier;

  std::vector<TypeEquation> type_equations;
  BoundVariables bound_variables;

  std::unordered_map<TypeHandle, bool, TypeHandle::Hash> registered_funcs;
  std::unordered_map<TypeHandle, bool, TypeHandle::Hash> registered_assignments;

  Instantiation instantiation;
};

}