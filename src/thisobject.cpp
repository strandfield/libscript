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
void ThisObject::init()
{
  // @TODO: see what we should do with this function
  throw std::runtime_error{ "call to ThisObject::init()" };
  // get().impl()->init_object();
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
  Value& self = get();

  size_t n = self.impl()->size();

  while (n > 0)
  {
    self.engine()->destroy(self.impl()->pop());
    --n;
  }
}

/*!
 * \fn template<typename T, typename...Args> void init<T>(Args &&...)
 * \brief Constructs an object in the value's memory buffer.
 *
 * The constructed object should be destroyed with a call to \m destroy<T>().
 */

/*!
 * \fn template<typename T> void destroy<T>()
 * \brief Destroys the object stored in the value's memory buffer.
 *
 * This function should be called if the Value was initialized with a call to \m init<T>().
 */

} // namespace script

