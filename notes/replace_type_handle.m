%{

instance.hpp
  using InstanceVariables = std::unordered_map<const Type*, const Type*>;
  
  - TypeHandle clone(const TypeHandle& handle, IV replacing);
  + TypeHandle clone(const Type* type, IV replacing);
    Note: replace with visitor method on Type.

library.hpp
  - Optional<TypeHandle> lookup_function(const FunctionDefHandle& def_handle) const;
  + Optional<const Type*> lookup_function(const FunctionDefHandle& def_handle) const;

  - void emplace_local_function_type(const FunctionDefHandle& handle, TypeRef type);
  + void emplace_local_function_type(const FunctionDefHandle& handle, const Type* type);

member_visitor.hpp
  class Predicate {
    - virtual bool predicate(TypeRef lhs, TypeRef rhs, bool rev) const = 0;
    + virtual bool predicate(Type* lhs, Type* rhs, bool rev) const = 0;

    - virtual bool parameters(TypeRef lhs, TypeRef rhs, const types::Parameters& a,
      const types::DestructuredTuple& b, int64_t offset_b, bool rev) const = 0;
    + virtual bool parameters(Type* lhs, Type* rhs, const types::Parameters& a,
      const types::DestructuredTuple& b, int64_t offset_b, bool rev) const = 0;
  };

  - bool expand_members(TypeRef lhs, TypeRef rhs, const DT& a, const DT& b, bool rev) const;
  + bool expand_members(Type* lhs, Type* rhs, const DT& a, const DT& b, bool rev) const;

simplify.hpp
  - bool simplify(TypeRef lhs, TypeRef rhs, bool rev);
  + bool simplify(Type* lhs, Type* rhs, bool rev);

type.hpp
  - using TypeHandles = std::vector<TypeHandle>;
  + using Types = std::vector<Type*>;

  struct TypeEquationTerm {
    ...
    - TypeHandle term;
    + Type* term;
  }

  * For each type, add a tag with its type and a default constructor to 
    initialize with that tag.
  * For each type, TypeHandle -> Type*

type_relation.hpp
  class TypeRelationship {
    - virtual bool related(TypeRef lhs, TypeRef rhs, const types::Scalar& a, const types::Scalar& b, bool rev) const = 0;
    + virtual bool related(const Type* lhs, const Type* rhs, const types::Scalar& a, const types::Scalar& b, bool rev) const = 0;
  } 
  
  - bool related_entry(const TypeHandle& a, const TypeHandle& b, bool rev = false) const;
  + bool related_entry(const Type* a, const Type* b, bool rev = false) const;

unification.hpp
  - MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term, types::Abstraction& func);
  + MT_NODISCARD TypeHandle apply_to(const Type* source, TermRef term, types::Abstraction& func);

  - MT_NODISCARD TypeHandle substitute_one(TypeRef source, TermRef term, TermRef lhs, TermRef rhs);
  + MT_NODISCARD TypeHandle substitute_one(const Type* source, TermRef term, TermRef lhs, TermRef rhs);

  - bool occurs(TypeRef t, TermRef term, TypeRef lhs) const;
  + bool occurs(const Type* t, TermRef term, const Type* lhs) const;

  
  
  
%}
