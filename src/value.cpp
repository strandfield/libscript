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

/*!
 * \class Value
 */

/*!
 * \fn Value()
 * \brief constructs an invalid, null value
 */
Value::Value()
  : d(nullptr)
{

}

/*!
 * \fn Value(const Value& other)
 * \brief constructs a new reference to a value
 * 
 * Note that because Value is implicitly shared, this does not copy 
 * the underlying value but rather the pointer to the value.
 * 
 * Use Engine::copy() to create a true copy of the value.
 */
Value::Value(const Value& other)
  : d(other.d)
{
  if (d)
    d->ref += 1;
}

/*!
 * \fn ~Value()
 * \brief destroys the reference to the value
 * 
 * If this instance was the last reference, the underlying value is 
 * destroyed.
 */
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

/*!
 * \fn bool isNull() const
 * \brief returns whether this instance does not reference a valid value
 */
bool Value::isNull() const
{
  return d == nullptr;
}

/*!
 * \fn Type type() const
 * \brief returns the value's type
 */
Type Value::type() const
{
  return d->type.withoutRef();
}

/*!
 * \fn bool isConst() const
 * \brief returns whether the value is marked as const
 * 
 * Note that this is just informative, the underlying value is never const 
 * and can always be modified programmatically.
 */
bool Value::isConst() const
{
  return type().isConst();
}

/*!
 * \fn bool isReference() const
 * \brief returns whether this instance stores a reference
 * 
 * When a Value stores a reference, it is not responsible for the lifetime 
 * of the actual object; the object should outlive the Value.
 */
bool Value::isReference() const
{
  return d->is_reference();
}

/*!
 * \fn bool isBool() const
 * \brief returns whether the value is a boolean
 * 
 * This is the C++ type \c bool.
 */
bool Value::isBool() const
{
  return d->type.baseType() == Type::Boolean;
}

/*!
 * \fn bool isChar() const
 * \brief returns whether the value is a character
 * 
 * This is the C++ type \c char.
 */
bool Value::isChar() const
{
  return d->type.baseType() == Type::Char;
}

/*!
 * \fn bool isInt() const
 * \brief returns whether the value is an integer.
 * 
 * This is the C++ type \c int.
 */
bool Value::isInt() const
{
  return d->type.baseType() == Type::Int;
}

/*!
 * \fn bool isFloat() const
 * \brief returns whether the value is a floating point number
 * 
 * This is the C++ type \c float.
 */
bool Value::isFloat() const
{
  return d->type.baseType() == Type::Float;
}

/*!
 * \fn bool isDouble() const
 * \brief returns whether the value is a double-precision floating point number
 * 
 * This is the C++ type \c double.
 */
bool Value::isDouble() const
{
  return d->type.baseType() == Type::Double;
}

/*!
 * \fn bool isPrimitive() const
 * \brief returns whether the value is of fundamental type
 * 
 * Fundamental types are bool, char, int, float, double.
 */
bool Value::isPrimitive() const
{
  return d->type.baseType().isFundamentalType();
}

/*!
 * \fn bool isString() const
 * \brief returns whether the value is a string
 */
bool Value::isString() const
{
  return d->type.baseType() == Type::String;
}

/*!
 * \fn bool isObject() const
 * \brief returns whether the value is an object
 */
bool Value::isObject() const
{
  return d->type.isObjectType();
}

/*!
 * \fn bool isArray() const
 * \brief returns whether the value is an array
 * 
 * See the Array class.
 */
bool Value::isArray() const
{
  return d->is_array();
}

/*!
 * \fn bool isInitializerList() const
 * \brief returns whether the value is an initializer list
 */
bool Value::isInitializerList() const
{
  return d->is_initializer_list();
}

/*!
 * \fn bool toBool() const
 * \brief returns the value as a boolean
 * 
 * Pre-condition: isBool() returns true.
 */
bool Value::toBool() const
{
  return get<bool>(*this);
}

/*!
 * \fn char toChar() const
 * \brief returns the value as a character
 *
 * Pre-condition: isChar() returns true.
 */
char Value::toChar() const
{
  return get<char>(*this);
}

/*!
 * \fn int toInt() const
 * \brief returns the value as an integer
 *
 * Pre-condition: isInt() returns true.
 */
int Value::toInt() const
{
  return get<int>(*this);
}

/*!
 * \fn float toFloat() const
 * \brief returns the value as a floating point number
 *
 * Pre-condition: isFloat() returns true.
 */
float Value::toFloat() const
{
  return get<float>(*this);
}

/*!
 * \fn double toDouble() const
 * \brief returns the value as a double-precision floating point number
 *
 * Pre-condition: isDouble() returns true.
 */
double Value::toDouble() const
{
  return get<double>(*this);
}

/*!
 * \fn String toString() const
 * \brief returns the value as a string
 *
 * Pre-condition: isString() returns true.
 */
String Value::toString() const
{
  return get<String>(*this);
}

/*!
 * \fn Function toFunction() const
 * \brief returns the value as a function
 *
 * If this value is not a function, this returns a null Function.
 */
Function Value::toFunction() const
{
  return d->is_function() ? static_cast<FunctionValue*>(d)->function : Function();
}

/*!
 * \fn Object toObject() const
 * \brief returns the value as an object
 *
 * If this value is not an object, this returns a null Object.
 */
Object Value::toObject() const
{
  return Object(*this);
}

/*!
 * \fn Array toArray() const
 * \brief returns the value as an array
 *
 * If this value is not an array, this returns a null Array.
 */
Array Value::toArray() const
{
  return d->is_array() ? static_cast<ArrayValue*>(d)->array : Array();
}

/*!
 * \fn Enumerator toEnumerator() const
 * \brief returns the value as an enumerator
 *
 * If this value is not an enumerator, this returns a null Enumerator.
 */
Enumerator Value::toEnumerator() const
{
  if (d->is_enumerator())
    return static_cast<EnumeratorValue*>(d)->value;
  else if (d->is_cpp_enum())
    return Enumerator(engine()->typeSystem()->getEnum(type()), d->get_cpp_enum_value());
  else
    return Enumerator();
}

/*!
 * \fn Lambda toLambda() const
 * \brief returns the value as a lambda
 *
 * If this value is not a lambda, this returns a null Lambda.
 */
Lambda Value::toLambda() const
{
  return d->is_lambda() ? static_cast<LambdaValue*>(d)->lambda : Lambda();
}

/*!
 * \fn InitializerList toInitializerList() const
 * \brief returns the value as an initializer list
 *
 * If this value is not an initializer list, this returns a null InitializerList.
 */
InitializerList Value::toInitializerList() const
{
  return d->is_initializer_list() ? static_cast<InitializerListValue*>(d)->initlist : InitializerList();
}

/*!
 * \fn void* data() const
 * \brief returns a pointer to the value's underlying data
 */
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

/*!
 * \fn Engine* engine() const
 * \brief returns a pointer to the script engine
 */
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

/*!
 * \endclass
 */

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

