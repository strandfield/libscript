// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/value.h"
#include "script/private/value_p.h"

#include "script/engine.h"
#include "script/private/engine_p.h"

#include "script/array.h"
#include "script/enumerator.h"
#include "script/function.h"
#include "script/initializerlist.h"
#include "script/object.h"
#include "script/private/object_p.h"

#if defined(LIBSCRIPT_CONFIG_VALUE_INJECTED_SOURCE)
#include LIBSCRIPT_CONFIG_VALUE_INJECTED_SOURCE
#endif // defined(LIBSCRIPT_CONFIG_VALUE_INJECTED_SOURCE)

namespace script
{

#if !defined(LIBSCRIPT_CONFIG_VALUEIMPL_HEADER)

ValueImpl::Storage::Storage()
{
  builtin.string = nullptr;
  initlistEnd = nullptr;
}

bool ValueImpl::is_object() const
{
  return !data.object.isNull();
}

const Object & ValueImpl::get_object() const
{
  return data.object;
}

void ValueImpl::init_object()
{
  if (!data.object.isNull())
    return;

  auto impl = std::make_shared<ObjectImpl>(this->engine->getClass(this->type));
  data.object = Object{ impl };
}

void ValueImpl::push_member(const Value & val)
{
  data.object.impl()->attributes.push_back(val);
}

Value ValueImpl::pop_member()
{
  auto ret = data.object.impl()->attributes.back();
  data.object.impl()->attributes.pop_back();
  return ret;
}

Value ValueImpl::get_member(size_t i) const
{
  return data.object.impl()->attributes.at(i);
}

bool ValueImpl::is_array() const
{
  return !data.array.isNull();
}

const Array & ValueImpl::get_array() const
{
  return data.array;
}

void ValueImpl::set_array(const Array & aval)
{
  data.array = aval;
}

bool ValueImpl::is_function() const
{
  return !data.function.isNull();
}

const Function & ValueImpl::get_function() const
{
  return data.function;
}

void ValueImpl::set_function(const Function & fval)
{
  data.function = fval;
}

bool ValueImpl::is_lambda() const
{
  return !data.lambda.isNull();
}

const Lambda & ValueImpl::get_lambda() const
{
  return data.lambda;
}

void ValueImpl::set_lambda(const Lambda & lval)
{
  data.lambda = lval;
}


const Enumerator & ValueImpl::get_enumerator() const
{
  return *data.builtin.enumValue;
}

void ValueImpl::set_enumerator(const Enumerator & val)
{
  if (data.builtin.enumValue == nullptr)
    data.builtin.enumValue = new Enumerator{ val };
  else
    *data.builtin.enumValue = val;
}

bool ValueImpl::is_initializer_list() const
{
  return data.initlistEnd != nullptr && data.builtin.valueptr != nullptr;
}

InitializerList ValueImpl::get_initializer_list() const
{
  return InitializerList{ data.builtin.valueptr, data.initlistEnd };
}

void ValueImpl::set_initializer_list(const InitializerList & il)
{
  data.builtin.valueptr = il.begin();
  data.initlistEnd = il.end();
}

void ValueImpl::clear()
{
  data.array = Array{};
  data.object = Object{};
  if (type.baseType() == Type::String)
  {
    delete data.builtin.string;
    data.builtin.string = nullptr;
  }
  else if (type.isEnumType())
  {
    delete data.builtin.enumValue;
    data.builtin.enumValue = nullptr;
  }
  this->type = Type::Null;
}

#endif // !defined(LIBSCRIPT_CONFIG_VALUEIMPL_HEADER)


static ValueImpl construct_void()
{
  ValueImpl vs{ Type::Void, nullptr };
  vs.ref = 1;
  return vs;
}

ValueImpl void_struct = construct_void();
const Value Value::Void = Value{ &void_struct };

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

Value::Value(ValueImpl * impl)
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
  return d->type;
}

bool Value::isConst() const
{
  return type().isConst();
}

bool Value::isInitialized() const
{
  return !type().testFlag(Type::UninitializedFlag);
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

bool Value::isObject() const
{
  return d->is_object();
}

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
  return d->get_bool();
}

char Value::toChar() const
{
  return d->get_char();
}

int Value::toInt() const
{
  return d->get_int();
}

float Value::toFloat() const
{
  return d->get_float();
}

double Value::toDouble() const
{
  return d->get_double();
}

String Value::toString() const
{
  return d->get_string();
}

Function Value::toFunction() const
{
  return d->get_function();
}

Object Value::toObject() const
{
  return d->get_object();
}

Array Value::toArray() const
{
  return d->get_array();
}

Enumerator Value::toEnumerator() const
{
  return d->get_enumerator();
}

Lambda Value::toLambda() const
{
  return d->get_lambda();
}

InitializerList Value::toInitializerList() const
{
  return d->get_initializer_list();
}

Value Value::fromEnumerator(const Enumerator & ev)
{
  if (ev.isNull())
    return Value{}; // TODO : should we throw
  Engine *e = ev.enumeration().engine();
  Value ret = e->implementation()->buildValue(ev.enumeration().id());
  ret.impl()->set_enumerator(ev);
  return ret;
}

Value Value::fromFunction(const Function & f, const Type & ft)
{
  if (f.isNull())
    return Value{}; // TODO : should we throw
  Engine *e = f.engine();
  Value ret = e->implementation()->buildValue(ft);
  ret.impl()->set_function(f);
  return ret;
}

Value Value::fromArray(const Array & a)
{
  if (a.isNull())
    return Value{};
  Engine *e = a.engine();
  Value ret = e->implementation()->buildValue(a.typeId());
  ret.impl()->set_array(a);
  return ret;
}

Value Value::fromLambda(const Lambda & obj)
{
  if (obj.isNull())
    return Value{};
  Engine *e = obj.engine();
  Value ret = e->implementation()->buildValue(obj.closureType().id());
  ret.impl()->set_lambda(obj);
  return ret;
}


Engine* Value::engine() const
{
  return d->engine;
}

bool Value::isManaged() const
{
  return d->type.testFlag(Type::ManagedFlag);
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

bool Value::operator==(const Value & other) const
{
  return d == other.d;
}

} // namespace script

