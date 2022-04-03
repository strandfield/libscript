// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/enum.h"
#include "script/private/enum_p.h"
#include "script/enumerator.h"

#include <algorithm>

#include "script/script.h"
#include "script/symbol.h"

#include "script/private/class_p.h"
#include "script/private/namespace_p.h"

namespace script
{

EnumImpl::EnumImpl(int i, const std::string & n, Engine *e)
  : engine(e)
  , id(i)
  , name(n)
  , enumClass(false)
{

}

/*!
 * \class Enum
 */

Enum::Enum(const std::shared_ptr<EnumImpl> & impl)
  : d(impl)
{

}

/*!
 * \fn int id() const
 * \brief returns the id of this enumeration
 */
int Enum::id() const
{
  return d->id;
}

/*!
 * \fn bool isNull() const
 * \brief returns whether this instance is null
 * 
 * Calling any other function than isNull() on a null instance 
 * is undefined behavior.
 */
bool Enum::isNull() const
{
  return d == nullptr;
}

/*!
 * \fn const std::string& name() const
 * \brief returns the enumeration's name
 */
const std::string& Enum::name() const
{
  return d->name;
}

/*!
 * \fn bool isEnumClass() const
 * \brief returns whether the enumeration is an enum class
 */
bool Enum::isEnumClass() const
{
  return d->enumClass;
}

/*!
 * \fn const std::map<std::string, int>& values() const
 * \brief returns the enumeration's value
 */
const std::map<std::string, int>& Enum::values() const
{
  return d->values;
}

/*!
 * \fn bool hasKey(const std::string& k) const
 * \brief returns whether there is a value with a given key in the enum
 */
bool Enum::hasKey(const std::string& k) const
{
  return d->values.find(k) != d->values.end();
}

/*!
 * \fn bool hasValue(int val) const
 * \brief returns whether the enum has a given value
 */
bool Enum::hasValue(int val) const
{
  for (const auto& pair : d->values)
  {
    if (pair.second == val)
      return true;
  }

  return false;
}

/*!
 * \fn int getValue(const std::string& k, int defaultValue = -1) const
 * \brief returns a value given its key
 * 
 * If there is no value with the given key, \a defaultValue is returned.
 */
int Enum::getValue(const std::string& k, int defaultValue) const
{
  auto it = d->values.find(k);
  if (it == d->values.end())
    return defaultValue;
  return it->second;
}

/*!
 * \fn const std::string& getKey(int val) const
 * \brief returns the key given its value
 * 
 * If there is none, this function throws an exception.
 */
const std::string& Enum::getKey(int val) const
{
  for (auto it = d->values.begin(); it != d->values.end(); ++it)
  {
    if (it->second == val)
      return it->first;
  }

  // @TODO: throw a dedicated exception
  throw std::runtime_error{ "Enum::getKey() : no such value" };
}

/*!
 * \fn int addValue(const std::string& key, int value)
 * \brief add a value to the enumeration
 */
int Enum::addValue(const std::string& key, int value)
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

/*!
 * \fn Class memberOf() const
 * \brief returns the class in which this enum was defined
 * 
 * If this enum wasn't defined in a class, this returns a null Class.
 */
Class Enum::memberOf() const
{
  Symbol s{ d ? d->enclosing_symbol.lock() : nullptr };
  return s.isClass() ? s.toClass() : Class();
}

/*!
 * \fn Namespace enclosingNamespace() const
 * \brief returns the namespace in which the enum is defined
 * 
 * If the enum was defined in a class, this returns the namespace in which the 
 * class was defined.
 */
Namespace Enum::enclosingNamespace() const
{
  Symbol s{ d ? d->enclosing_symbol.lock() : nullptr };

  if (s.isClass())
    return s.toClass().enclosingNamespace();
  else if (s.isNamespace())
    return s.toNamespace();
  else
    return {};
}

/*!
 * \fn Engine* engine() const
 * \brief returns the script engine
 */
Engine* Enum::engine() const
{
  return d->engine;
}

/*!
 * \fn Script script() const
 * \brief returns the script in which this enum was defined
 */
Script Enum::script() const
{
  return Symbol(d ? d->enclosing_symbol.lock() : nullptr).script();
}

/*!
 * \endclass
 */

} // namespace script
