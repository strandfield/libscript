// Copyright (C) 2019 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/locals.h"

#include "script/engine.h"

#include "script/private/value_p.h"

namespace script
{

/*!
 * \class Locals
 * \brief Manages a set of local variables
 */

/*!
 * \fn Locals()
 */

/*!
 * \fn Locals(Locals&& other)
 * \brief Move constructor
 *
 * Transfer ownership of all variables in \a other 
 * to the newly constructed Locals object. 
 * No copy is performed.
 */
Locals::Locals(Locals&& other)
  : m_values(std::move(other.m_values))
{

}

/*!
 * \fn ~Locals()
 * \brief Destructor
 *
 * Destroys all variables in this Locals object that have 
 * a reference counter equal to 1.
 */
Locals::~Locals()
{
  destroy();
}

/*!
 * \fn void push(const Value& val)
 * \brief Adds a value to this set of locals
 *
 * The Value is not copied. The Locals object takes ownership of the Value 
 * if its reference count is 1.
 */
void Locals::push(const Value& val)
{
  m_values.push_back(val);
}

/*!
 * \fn void pop()
 * \brief Removes the last value added to this set of locals
 *
 * The Value is destroyed if it is owned by the Locals object.
 */
void Locals::pop()
{
  Value v = take();

  if (v.impl()->ref == 1)
  {
    v.engine()->destroy(v);
  }
}

/*!
 * \fn Value take()
 * \brief Removes the last value added to this set of locals
 *
 * Unlike \m pop(), the Value is not destroyed; ownership is transferred back 
 * to the caller of this function.
 */
Value Locals::take()
{
  Value v = m_values.back();
  m_values.pop_back();
  return v;
}

/*!
 * \fn size_t size() const
 * \brief Returns the number of variables in this object
 */
size_t Locals::size() const
{
  return data().size();
}

/*!
 * \fn const Value& at(size_t index) const
 * \brief Retrieves a variable in this object
 */
const Value& Locals::at(size_t index) const
{
  return data().at(index);
}

/*!
 * \fn const std::vector<Value>& data() const
 * \brief Returns the internal data
 */
const std::vector<Value>& Locals::data() const
{
  return m_values;
}

/*!
 * \fn Locals& operator=(Locals&& other)
 * \brief Assignment operator
 *
 * Transfer ownership of all Values in \a other to this Locals object.
 */
Locals& Locals::operator=(Locals&& other)
{
  destroy();
  m_values = std::move(other.m_values);
  return *this;
}

/*!
 * \fn LocalsProxy operator[](size_t index)
 * \brief Provides write access to the locals' variables
 *
 */
LocalsProxy Locals::operator[](size_t index)
{
  return LocalsProxy(*this, index);
}

/*!
 * \fn const Value& operator[](size_t index) const
 * \brief Array-like access
 *
 * This is the same as \m at().
 */
const Value& Locals::operator[](size_t index) const
{
  return at(index);
}

void Locals::destroy()
{
  for (auto& v : m_values)
  {
    if (v.impl()->ref == 1)
    {
      v.engine()->destroy(v);
    }
  }

  m_values.clear();
}

LocalsProxy::LocalsProxy(Locals& locals, size_t index)
  : l(&locals),
    i(index)
{

}

LocalsProxy& LocalsProxy::operator=(const LocalsProxy& other)
{
  if (l->m_values.at(i).impl()->ref == 1)
  {
    Value v = l->m_values[i];
    v.engine()->destroy(v);
  }

  const Value& val = other.l->at(other.i);
  l->m_values[i] = val;

  return *this;
}

LocalsProxy& LocalsProxy::operator=(const Value& value)
{
  if (l->m_values.at(i).impl()->ref == 1)
  {
    Value v = l->m_values[i];
    v.engine()->destroy(v);
  }

  l->m_values[i] = value;

  return *this;
}

LocalsProxy::operator const Value& () const
{
  return l->at(i);
}

} // namespace script
