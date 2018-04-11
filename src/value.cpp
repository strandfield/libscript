// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/value.h"
#include "value_p.h"

#include "script/engine.h"
#include "engine_p.h"

#include "script/array.h"
#include "script/enumvalue.h"
#include "script/function.h"
#include "script/object.h"

namespace script
{

ValueStruct::Storage::Storage()
{
  builtin.string = nullptr;
}

bool ValueStruct::isObject() const
{
  return !data.object.isNull();
}

const Object & ValueStruct::getObject() const
{
  return data.object;
}
void ValueStruct::setObject(const Object & oval)
{
  data.object = oval;
}

bool ValueStruct::isArray() const
{
  return !data.array.isNull();
}

const Array & ValueStruct::getArray() const
{
  return data.array;
}

void ValueStruct::setArray(const Array & aval)
{
  data.array = aval;
}

bool ValueStruct::isFunction() const
{
  return !data.function.isNull();
}

const Function & ValueStruct::getFunction() const
{
  return data.function;
}

void ValueStruct::setFunction(const Function & fval)
{
  data.function = fval;
}

bool ValueStruct::isLambda() const
{
  return !data.lambda.isNull();
}

const LambdaObject & ValueStruct::getLambda() const
{
  return data.lambda;
}

void ValueStruct::setLambda(const LambdaObject & lval)
{
  data.lambda = lval;
}


const EnumValue & ValueStruct::getEnumValue() const
{
  return *data.builtin.enumValue;
}

void ValueStruct::setEnumValue(const EnumValue & evval)
{
  if (data.builtin.enumValue == nullptr)
    data.builtin.enumValue = new EnumValue{ evval };
  else
    *data.builtin.enumValue = evval;
}

void ValueStruct::clear()
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

static ValueStruct construct_void()
{
  ValueStruct vs{ Type::Void, nullptr };
  vs.ref = 1;
  return vs;
}

ValueStruct void_struct = construct_void();
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

Value::Value(ValueStruct * impl)
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
  return d->isObject();
}

bool Value::isArray() const
{
  return d->isArray();
}

bool Value::toBool() const
{
  return d->getBool();
}

char Value::toChar() const
{
  return d->getChar();
}

int Value::toInt() const
{
  return d->getInt();
}

float Value::toFloat() const
{
  return d->getFloat();
}

double Value::toDouble() const
{
  return d->getDouble();
}

String Value::toString() const
{
  return d->getString();
}

Function Value::toFunction() const
{
  return d->getFunction();
}

Object Value::toObject() const
{
  return d->getObject();
}

Array Value::toArray() const
{
  return d->getArray();
}

EnumValue Value::toEnumValue() const
{
  return d->getEnumValue();
}

LambdaObject Value::toLambda() const
{
  return d->getLambda();
}

Value Value::fromEnumValue(const EnumValue & ev)
{
  if (ev.isNull())
    return Value{}; // TODO : should we throw
  Engine *e = ev.enumeration().engine();
  Value ret = e->implementation()->buildValue(ev.enumeration().id());
  ret.impl()->setEnumValue(ev);
  return ret;
}

Value Value::fromFunction(const Function & f, const Type & ft)
{
  if (f.isNull())
    return Value{}; // TODO : should we throw
  Engine *e = f.engine();
  Value ret = e->implementation()->buildValue(ft);
  ret.impl()->setFunction(f);
  return ret;
}

Value Value::fromArray(const Array & a)
{
  if (a.isNull())
    return Value{};
  Engine *e = a.engine();
  Value ret = e->implementation()->buildValue(a.typeId());
  ret.impl()->setArray(a);
  return ret;
}

Value Value::fromObject(const Object & obj)
{
  if (obj.isNull())
    return Value{};
  Engine *e = obj.engine();
  Value ret = e->implementation()->buildValue(obj.instanceOf().id());
  ret.impl()->setObject(obj);
  return ret;
}

Value Value::fromLambda(const LambdaObject & obj)
{
  if (obj.isNull())
    return Value{};
  Engine *e = obj.engine();
  Value ret = e->implementation()->buildValue(obj.closureType().id());
  ret.impl()->setLambda(obj);
  return ret;
}


Engine* Value::engine() const
{
  return d->engine;
}

bool Value::isManaged() const
{
  return d->isManaged();
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

