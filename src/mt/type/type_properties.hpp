#pragma once

#include "types.hpp"
#include <unordered_set>

namespace mt {

/*
 * IsConcreteArgument
 */

class IsConcreteArgument {
public:
  IsConcreteArgument() = delete;

  static bool is_concrete_argument(const Type* arg);
  static bool are_concrete_arguments(const TypePtrs& args);

private:
  static bool is_concrete_argument(const types::Scheme& scheme);
  static bool is_concrete_argument(const types::Abstraction& abstr);
  static bool is_concrete_argument(const types::DestructuredTuple& tup);
  static bool is_concrete_argument(const types::List& list);
  static bool is_concrete_argument(const types::Union& union_type);
  static bool is_concrete_argument(const types::Class& class_type);
  static bool is_concrete_argument(const types::Alias& alias);
};

/*
 * IsFullyConcrete
 */

struct IsFullyConcreteInstance {
  bool maybe_push_scheme_variables(const Type* in_type);

  std::unordered_set<const Type*> unbound_variables;
  std::vector<std::unordered_set<const Type*>> enclosing_scheme_variables;
};

class IsFullyConcrete {
public:
  IsFullyConcrete(IsFullyConcreteInstance* instance);

  bool scheme(const types::Scheme& scheme) const;
  bool abstraction(const types::Abstraction& abstr) const;
  bool application(const types::Application& app) const;
  bool constant_value(const types::ConstantValue& cv) const;
  bool variable(const types::Variable& var) const;
  bool parameters(const types::Parameters& params) const;
  bool list(const types::List& list) const;
  bool tuple(const types::Tuple& tup) const;
  bool destructured_tuple(const types::DestructuredTuple& tup) const;
  bool union_type(const types::Union& union_type) const;
  bool alias(const types::Alias& alias) const;
  bool record(const types::Record& record) const;
  bool class_type(const types::Class& class_type) const;
  bool scalar(const types::Scalar& scalar) const;
  bool subscript(const types::Subscript& sub) const;
  bool assignment(const types::Assignment& assign) const;

private:
  bool type_ptrs(const TypePtrs& types) const;
  void two_types(const Type* a, const Type* b, bool* success) const;

private:
  IsFullyConcreteInstance* instance;
};

}