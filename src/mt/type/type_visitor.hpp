#pragma once

#define MT_TYPE_VISITOR_METHOD(name, type) \
  virtual void name(type&) {} \
  virtual void name(const type&) {}

namespace mt {

namespace types {
  struct Scalar;
  struct Variable;
  struct Abstraction;
  struct Application;
  struct Assignment;
  struct DestructuredTuple;
  struct Tuple;
  struct List;
  struct Union;
  struct Subscript;
  struct Scheme;
  struct Parameters;
  struct ConstantValue;
  struct Class;
  struct Record;
  struct Alias;
  struct Cast;
}

class TypeVisitor {
public:
  MT_TYPE_VISITOR_METHOD(scalar, types::Scalar)
  MT_TYPE_VISITOR_METHOD(variable, types::Variable)
  MT_TYPE_VISITOR_METHOD(abstraction, types::Abstraction)
  MT_TYPE_VISITOR_METHOD(application, types::Application)
  MT_TYPE_VISITOR_METHOD(assignment, types::Assignment)
  MT_TYPE_VISITOR_METHOD(destructured_tuple, types::DestructuredTuple)
  MT_TYPE_VISITOR_METHOD(tuple, types::Tuple)
  MT_TYPE_VISITOR_METHOD(list, types::List)
  MT_TYPE_VISITOR_METHOD(union_type, types::Union)
  MT_TYPE_VISITOR_METHOD(subscript, types::Subscript)
  MT_TYPE_VISITOR_METHOD(scheme, types::Scheme)
  MT_TYPE_VISITOR_METHOD(parameters, types::Parameters)
  MT_TYPE_VISITOR_METHOD(constant_value, types::ConstantValue)
  MT_TYPE_VISITOR_METHOD(class_type, types::Class)
  MT_TYPE_VISITOR_METHOD(record, types::Record)
  MT_TYPE_VISITOR_METHOD(alias, types::Alias)
  MT_TYPE_VISITOR_METHOD(cast, types::Cast)
};

#undef MT_TYPE_VISITOR_METHOD

}