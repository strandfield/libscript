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

  data_ptr = &(data.string);
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

    auto impl = std::make_shared<ObjectImpl>(this->engine->typeSystem()->getClass(this->type));
    new (&data.object) Object{ impl };

    which = ObjectField;
  }
  else
  {
    if (!data.object.isNull())
      return;

    auto impl = std::make_shared<ObjectImpl>(this->engine->typeSystem()->getClass(this->type));
    data.object = Object{ impl };
  }

  data_ptr = &(data.object);
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

bool ValueImpl::is_function() const
{
  return which == FunctionField && !data.function.isNull();
}

const Function& ValueImpl::get_function() const
{
  assert(which == FunctionField);

  return *static_cast<Function*>(data_ptr);
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

  data_ptr = &(data.function);
}

bool ValueImpl::is_lambda() const
{
  return which == LambdaField && !data.lambda.isNull();
}

const Lambda& ValueImpl::get_lambda() const
{
  assert(which == LambdaField);

  return *static_cast<Lambda*>(data_ptr);
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

  data_ptr = &(data.lambda);
}

const Enumerator& ValueImpl::get_enumerator() const
{
  assert(which == EnumeratorField);

  return *static_cast<Enumerator*>(data_ptr);
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

  data_ptr = &(data.enumerator);
}

bool ValueImpl::is_initializer_list() const
{
  return which == InitListField;
}

InitializerList ValueImpl::get_initializer_list() const
{
  assert(which == InitListField);

  return *static_cast<InitializerList*>(data_ptr);
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

  data_ptr = &(data.initializer_list);
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
  data_ptr = &(data.memory);
  return &(data.memory);
}

void ValueImpl::release_memory()
{
  assert(which == MemoryField);
  data.fundamentals.boolean = false;
  which = FundamentalsField;
}

void* ValueImpl::get_data() const
{
  return data_ptr;
}

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
  // @TODO: if 'd' is a CppValue that is a reference, we could do the conversion
  return d->is_enumerator() ? static_cast<EnumeratorValue*>(d)->value : Enumerator();
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
  return Value(new EnumeratorValue(ev));
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

namespace details
{

int get_enum_value(const Value& val)
{
  return val.toEnumerator().value();
}

} // namespace details

} // namespace script

