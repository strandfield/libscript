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

#include <cstring>

namespace script
{

ValueImpl::ValueImpl(Type t, Engine* e) : ref(0), type(t), engine(e)
{
  std::memset(data.memory, 0, Value::MemoryBufferSize);
  which = FundamentalsField;
}

ValueImpl::Data::Data()
{
  fundamentals.boolean = false;
}

ValueImpl::Data::~Data()
{
  fundamentals.boolean = false;
}

ValueImpl::ValueImpl(const ValueImpl& other)
  : ref(other.ref), type(other.type), engine(other.engine)
{
  assert(type.isNull() || type == Type::Void);
}

ValueImpl::~ValueImpl()
{
  clear();
}

String& ValueImpl::get_string()
{
  assert(this->which == StringField);
  return data.string;
}

void ValueImpl::set_string(const String& sval)
{
  if (this->which != StringField)
  {
    clear();
    new (&data.string) String{ sval };
    which = StringField;
  }
  else
  {
    data.string = sval;
  }
}

bool ValueImpl::is_object() const
{
  return which == ObjectField && !data.object.isNull();
}

const Object& ValueImpl::get_object() const
{
  assert(which == ObjectField);

  return data.object;
}

void ValueImpl::init_object()
{
  if (which != ObjectField)
  {
    clear();

    auto impl = std::make_shared<ObjectImpl>(this->engine->getClass(this->type));
    new (&data.object) Object{ impl };

    which = ObjectField;
  }
  else
  {
    if (!data.object.isNull())
      return;

    auto impl = std::make_shared<ObjectImpl>(this->engine->getClass(this->type));
    data.object = Object{ impl };
  }
}

void ValueImpl::push_member(const Value & val)
{
  assert(which == ObjectField);

  data.object.push(val);
}

Value ValueImpl::pop_member()
{
  assert(which == ObjectField);

  return data.object.pop();
}

Value ValueImpl::get_member(size_t i) const
{
  assert(which == ObjectField);

  return data.object.at(i);
}

size_t ValueImpl::member_count() const
{
  assert(which == ObjectField);

  return data.object.size();
}


bool ValueImpl::is_array() const
{
  return which == ArrayField && !data.array.isNull();
}

const Array& ValueImpl::get_array() const
{
  assert(which == ArrayField);

  return data.array;
}

void ValueImpl::set_array(const Array & aval)
{
  if (which != ArrayField)
  {
    clear();

    new (&data.array) Array{ aval };

    which = ArrayField;
  }
  else
  {
    data.array = aval;
  }
}

#if defined(LIBSCRIPT_USE_BUILTIN_STRING_BACKEND)

bool ValueImpl::is_charref() const
{
  return which == CharrefField;
}

CharRef& ValueImpl::get_charref()
{
  assert(which == CharrefField);

  return data.charref;
}

void ValueImpl::set_charref(const CharRef & cr)
{
  if (which != CharrefField)
  {
    clear();

    new (&data.charref) CharRef{ cr };

    which = CharrefField;
  }
  else
  {
    data.charref = cr;
  }
}

#endif // defined(LIBSCRIPT_USE_BUILTIN_STRING_BACKEND)

bool ValueImpl::is_function() const
{
  return which == FunctionField && !data.function.isNull();
}

const Function& ValueImpl::get_function() const
{
  assert(which == FunctionField);

  return data.function;
}

void ValueImpl::set_function(const Function & fval)
{
  if (which != FunctionField)
  {
    clear();

    new (&data.function) Function{ fval };

    which = FunctionField;
  }
  else
  {
    data.function = fval;
  }
}

bool ValueImpl::is_lambda() const
{
  return which == LambdaField && !data.lambda.isNull();
}

const Lambda& ValueImpl::get_lambda() const
{
  assert(which == LambdaField);

  return data.lambda;
}

void ValueImpl::set_lambda(const Lambda & lval)
{
  if (which != LambdaField)
  {
    clear();

    new (&data.lambda) Lambda{ lval };

    which = LambdaField;
  }
  else
  {
    data.lambda = lval;
  }
}

const Enumerator& ValueImpl::get_enumerator() const
{
  assert(which == EnumeratorField);

  return data.enumerator;
}

void ValueImpl::set_enumerator(const Enumerator & en)
{
  if (which != EnumeratorField)
  {
    clear();

    new (&data.enumerator) Enumerator{ en };

    which = EnumeratorField;
  }
  else
  {
    data.enumerator = en;
  }
}

bool ValueImpl::is_initializer_list() const
{
  return which == InitListField;
}

InitializerList ValueImpl::get_initializer_list() const
{
  assert(which == InitListField);

  return data.initializer_list;
}

void ValueImpl::set_initializer_list(const InitializerList & il)
{
  if (which != InitListField)
  {
    clear();

    new (&data.initializer_list) InitializerList{ il };

    which = InitListField;
  }
  else
  {
    data.initializer_list = il;
  }
}

void ValueImpl::clear()
{
  assert(which != MemoryField);

  switch (which)
  {
  case FundamentalsField:
    return;
  case StringField:
    data.string.~String();
    break;
  case ObjectField:
    data.object.~Object();
    break;
  case ArrayField:
    data.array.~Array();
    break;
  case FunctionField:
    data.function.~Function();
    break;
  case LambdaField:
    data.lambda.~Lambda();
    break;
  case EnumeratorField:
    data.enumerator.~Enumerator();
    break;
#if defined(LIBSCRIPT_USE_BUILTIN_STRING_BACKEND)
  case CharrefField:
    data.charref.~CharRef();
    break;
#endif // defined(LIBSCRIPT_USE_BUILTIN_STRING_BACKEND)
  case InitListField:
    data.initializer_list.~InitializerList();
    break;
  }

  data.fundamentals.boolean = false;
  which = FundamentalsField;
}

void* ValueImpl::acquire_memory()
{
  if (which != ValueImpl::MemoryField)
    clear();

  which = ValueImpl::MemoryField;
  return &(data.memory);
}

void ValueImpl::release_memory()
{
  assert(which == MemoryField);
  data.fundamentals.boolean = false;
  which = FundamentalsField;
}

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

void* Value::memory() const
{
  assert(d->which == ValueImpl::MemoryField);

  return &(d->data.memory);
}

Value Value::fromEnumerator(const Enumerator & ev)
{
  if (ev.isNull())
    return Value{}; // TODO : should we throw
  Engine *e = ev.enumeration().engine();
  Value ret = e->allocate(ev.enumeration().id());
  ret.impl()->set_enumerator(ev);
  return ret;
}

Value Value::fromFunction(const Function & f, const Type & ft)
{
  if (f.isNull())
    return Value{}; // TODO : should we throw
  Engine *e = f.engine();
  Value ret = e->allocate(ft);
  ret.impl()->set_function(f);
  return ret;
}

Value Value::fromArray(const Array & a)
{
  if (a.isNull())
    return Value{};
  Engine *e = a.engine();
  Value ret = e->allocate(a.typeId());
  ret.impl()->set_array(a);
  return ret;
}

Value Value::fromLambda(const Lambda & obj)
{
  if (obj.isNull())
    return Value{};
  Engine *e = obj.engine();
  Value ret = e->allocate(obj.closureType().id());
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

void* Value::acquireMemory()
{
  return d->acquire_memory();
}

void Value::releaseMemory()
{
  d->release_memory();
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

namespace details
{

int get_enum_value(const Value& val)
{
    return val.impl()->get_enumerator().value();
}

} // namespace details

/* get<T>() specializations */

template<>
bool& get<bool>(const Value& val)
{
  return val.impl()->get_bool();
}

template<>
char& get<char>(const Value& val)
{
  return val.impl()->get_char();
}

template<>
int& get<int>(const Value& val)
{
  return val.impl()->get_int();
}

template<>
float& get<float>(const Value& val)
{
  return val.impl()->get_float();
}

template<>
double& get<double>(const Value& val)
{
  return val.impl()->get_double();
}

template<>
String& get<String>(const Value& val)
{
  return val.impl()->get_string();
}

} // namespace script

