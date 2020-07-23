// Copyright (C) 2019 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/thisobject.h"

#include "script/engine.h"

#include "script/private/value_p.h"

namespace script
{

/*!
 * \class ThisObject
 * \brief Convenience class providing initialization/destruction utilities.
 *
 * This class is a helper class that is used during \t Value initialization and 
 * destruction. 
 * Instances of this class are returned by the \m thisObject() member of \t FunctionCall.
 */

/*!
 * \fn void init()
 * \brief Initialize the object
 *
 * After initialization, the \m toObject() method of \t Value returns a valid \t Object.
 */
void ThisObject::init(script::Type t)
{
  m_value = Value(new ScriptValue(m_engine, t));
}

/*!
 * \fn void push(const Value& val)
 * \brief Adds a data member to the object
 *
 * This method should be called after \m init().
 */
void ThisObject::push(const Value& val)
{
  get().impl()->push(val);
}

/*!
 * \fn Value pop()
 * \brief Takes ownership of the latest data member.
 */
Value ThisObject::pop()
{
  return get().impl()->pop();
}

/*!
 * \fn void destroy()
 * \brief Destroys all data members of the object.
 */
void ThisObject::destroy()
{
  while (get().impl()->size() > 0)
    get().impl()->pop();
}

} // namespace script

