// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/value.h"
#include "script/private/value_p.h"

#include "script/engine.h"
#include "script/array.h"
#include "script/enumerator.h"
#include "script/function.h"
#include "script/initializerlist.h"
#include "script/object.h"
#include "script/typesystem.h"

#include "script/private/engine_p.h"
#include "script/private/enum_p.h"
#include "script/private/object_p.h"

#include <cstring>

namespace script
{

const Value Value::Void = Value{ new VoidValue() };

IValue::~IValue()
{

}

bool IValue::is_void() const
{
  return false;
}

bool IValue::is_reference() const
{
  return false;
}

bool IValue::is_function() const
{
  return false;
}

bool IValue::is_lambda() const
{
  return false;
}

bool IValue::is_array() const
{
  return false;
}

bool IValue::is_initializer_list() const
{
  return false;
}

bool IValue::is_enumerator() const
{
  return false;
}

bool IValue::is_cpp_enum() const
{
  return false;
}

int IValue::get_cpp_enum_value() const
{
  return -1;
}

size_t IValue::size() const
{
  return 0;
}

void IValue::push(const Value& val)
{

}

Value IValue::pop()
{
  throw std::runtime_error{ "Value does not have any member" };
}

Value& IValue::at(size_t index)
{
  throw std::runtime_error{ "Value does not have any member" };
}


Value::Value()
  : d(nullptr)
{

}

Value::Value(const Value & other)
  : d(other.d)
{
  if (d)
    d->ref += 1;
}

Value::~Value()
{
  if (d)
  {
    if (--(d->ref) == 0)
    {
      /// TODO : add a check for non-destructed objects (i.e. potential memory leaks)
      delete d;
    }
  }
}

Value::Value(IValue* impl)
  : d(impl)
{
  if (d)
    d->ref += 1;
}


bool Value::isNull() const
{
  return d == nullptr;
}

Type Value::type() const
{
  return d->type.withoutRef();
}

bool Value::isConst() const
{
  return type().isConst();
}

bool Value::isReference() const
{
  return d->is_reference();
}

bool Value::isBool() const
{
  return d->type.baseType() == Type::Boolean;
}

bool Value::isChar() const
{
  return d->type.baseType() == Type::Char;
}

bool Value::isInt() const
{
  return d->type.baseType() == Type::Int;
}

bool Value::isFloat() const
{
  return d->type.baseType() == Type::Float;
}

bool Value::isDouble() const
{
  return d->type.baseType() == Type::Double;
}

bool Value::isPrimitive() const
{
  return d->type.baseType().isFundamentalType();
}

bool Value::isString() const
{
  return d->type.baseType() == Type::String;
}

//bool Value::isObject() const
//{
//  return d->is_object();
//}

bool Value::isArray() const
{
  return d->is_array();
}

bool Value::isInitializerList() const
{
  return d->is_initializer_list();
}

bool Value::toBool() const
{
  return get<bool>(*this);
}

char Value::toChar() const
{
  return get<char>(*this);
}

int Value::toInt() const
{
  return get<int>(*this);
}

float Value::toFloat() const
{
  return get<float>(*this);
}

double Value::toDouble() const
{
  return get<double>(*this);
}

String Value::toString() const
{
  return get<String>(*this);
}

Function Value::toFunction() const
{
  return d->is_function() ? static_cast<FunctionValue*>(d)->function : Function();
}

//Object Value::toObject() const
//{
//  return d->get_object();
//}

Array Value::toArray() const
{
  return d->is_array() ? static_cast<ArrayValue*>(d)->array : Array();
}

Enumerator Value::toEnumerator() const
{
  if (d->is_enumerator())
    return static_cast<EnumeratorValue*>(d)->value;
  else if (d->is_cpp_enum())
    return Enumerator(engine()->typeSystem()->getEnum(type()), d->get_cpp_enum_value());
  else
    return Enumerator();
}

Lambda Value::toLambda() const
{
  return d->is_lambda() ? static_cast<LambdaValue*>(d)->lambda : Lambda();
}

InitializerList Value::toInitializerList() const
{
  return d->is_initializer_list() ? static_cast<InitializerListValue*>(d)->initlist : InitializerList();
}

void* Value::data() const
{
  return d->ptr();
}

void* Value::ptr() const
{
  return d->ptr();
}

Value Value::fromEnumerator(const Enumerator & ev)
{
  if (ev.isNull())
    return Value{}; // TODO : should we throw
  // An enumerator might be represented by a C++ enum and not by Enumerator
  Enum enm = ev.enumeration();
  Value arg = enm.engine()->newInt(ev.value());
  return enm.impl()->from_int.invoke({ arg });
}

Value Value::fromFunction(const Function & f, const Type & ft)
{
  if (f.isNull())
    return Value{}; // TODO : should we throw
  return Value(new FunctionValue(f, ft));
}

Value Value::fromArray(const Array & a)
{
  if (a.isNull())
    return Value{};
  return Value(new ArrayValue(a));
}

Value Value::fromLambda(const Lambda & obj)
{
  if (obj.isNull())
    return Value{};
  return Value(new LambdaValue(obj));
}

Engine* Value::engine() const
{
  return d->engine;
}

Value & Value::operator=(const Value & other)
{
  auto *dd = d;
  d = other.d;
  if(d != nullptr)
    d->ref += 1;
  if (dd != nullptr)
  {
    if (--(dd->ref) == 0)
      delete dd;
  }
  return *(this);
}

namespace details
{

int get_enum_value(const Value& val)
{
  return val.toEnumerator().value();
}

} // namespace details

/* get<T>() specializations */

template<>
Function& get<Function>(const Value& val)
{
  assert(val.impl()->is_function());
  return static_cast<FunctionValue*>(val.impl())->function;
}

template<>
Array& get<Array>(const Value& val)
{
  assert(val.impl()->is_array());
  return static_cast<ArrayValue*>(val.impl())->array;
}

template<>
Enumerator& get<Enumerator>(const Value& val)
{
  assert(val.impl()->is_enumerator());
  return static_cast<EnumeratorValue*>(val.impl())->value;
}

template<>
Lambda& get<Lambda>(const Value& val)
{
  assert(val.impl()->is_lambda());
  return static_cast<LambdaValue*>(val.impl())->lambda;
}

template<>
InitializerList& get<InitializerList>(const Value& val)
{
  assert(val.impl()->is_initializer_list());
  return static_cast<InitializerListValue*>(val.impl())->initlist;
}

} // namespace script

