// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/enum.h"
#include "script/private/enum_p.h"
#include "script/enumvalue.h"

#include <algorithm>

#include "script/script.h"

#include "script/private/operator_p.h"
#include "script/private/value_p.h"
#include "script/interpreter/executioncontext.h"

namespace script
{

namespace callbacks
{

Value enum_assignment(interpreter::FunctionCall *c)
{
  c->arg(0).impl()->set_enum_value(c->arg(1).toEnumValue());
  return c->arg(0);
}

} // namespace callbacks

EnumImpl::EnumImpl(int i, const std::string & n, Engine *e)
  : engine(e)
  , id(i)
  , name(n)
  , enumClass(false)
{
  Prototype proto{ Type::ref(id), Type::ref(id), Type::cref(id) };
  auto op = std::make_shared<OperatorImpl>(Operator::AssignmentOperator, proto, e);
  op->implementation.callback = callbacks::enum_assignment;
  this->assignment = Operator{ op };
}

Enum::Enum(const std::shared_ptr<EnumImpl> & impl)
  : d(impl)
{

}


int Enum::id() const
{
  return d->id;
}

bool Enum::isNull() const
{
  return d == nullptr;
}

const std::string & Enum::name() const
{
  return d->name;
}

bool Enum::isEnumClass() const
{
  return d->enumClass;
}

const std::map<std::string, int> & Enum::values() const
{
  return d->values;
}

bool Enum::hasKey(const std::string & k) const
{
  return d->values.find(k) != d->values.end();
}

bool Enum::hasValue(int val) const
{
  for (const auto & pair : d->values)
  {
    if (pair.second == val)
      return true;
  }

  return false;
}

int Enum::getValue(const std::string & k, int defaultValue) const
{
  auto it = d->values.find(k);
  if (it == d->values.end())
    return defaultValue;
  return it->second;
}

int Enum::addValue(const std::string & key, int value)
{
  if (value == -1)
  {
    for (const auto & pair : d->values)
      value = std::max(pair.second, value);
    value += 1;
  }

  d->values[key] = value;
  return value;
}

Operator Enum::getAssignmentOperator() const
{
  return d->assignment;
}


Class Enum::memberOf() const
{
  return Class{ d->enclosing_class.lock() };
}

Namespace Enum::enclosingNamespace() const
{
  Class c = memberOf();
  if (c.isNull())
    return Namespace{ d->enclosing_namespace.lock() };
  return c.enclosingNamespace();
}

bool Enum::operator==(const Enum & other) const
{
  return d == other.d;
}

Engine * Enum::engine() const
{
  return d->engine;
}

Script Enum::script() const
{
  return Script{ d->script.lock() };
}

} // namespace script
