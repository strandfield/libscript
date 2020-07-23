// Copyright (C) 2018-2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/object.h"

#include "script/class.h"
#include "script/engine.h"
#include "script/typesystem.h"

namespace script
{

/*!
 * \class Object
 * \brief provides functions for Value that are objects
 *
 * The Object class provides a uniform way to access the data members of an object.
 *
 * An Object can be constructed from any Value. If the Value represents an Object, 
 * isValid() will return true.
 *
 */

/*!
 * \fn Object()
 * \brief constructs a null object
 */

/*!
 * \fn Object(const Object& other)
 * \brief copy constructor
 *
 * Warning: this does not copy the underlying object.
 */


/*!
 * \fn Object(const Value& val)
 * \brief constructs an object from a value
 *
 */
Object::Object(const Value& val)
  : d(val.isObject() ? val : Value())
{

}

/*!
 * \fn bool isNull() const
 * \brief returns whether the object is null
 *
 */
bool Object::isNull() const
{
  return d.isNull();
}

/*!
 * \fn Class instanceOf() const
 * \brief returns the class associated with the object
 *
 */
Class Object::instanceOf() const
{
  return d.engine()->typeSystem()->getClass(d.type());
}

/*!
 * \fn const Value& at(size_t i) const
 * \brief returns the data-member at the given offset
 *
 */
const Value& Object::at(size_t i) const
{
  return d.impl()->at(i);
}

/*!
 * \fn size_t size() const
 * \brief returns the actual number of data-member in the object
 *
 */
size_t Object::size() const
{
  return d.impl()->size();
}

/*!
 * \fn Value get(const std::string & attrName) const
 * \brief returns the data-member with the given name
 *
 * If no such data member exists, this returns a default constructed \t Value.
 */
Value Object::get(const std::string & attrName) const
{
  Class c = instanceOf();
  int index = c.attributeIndex(attrName);
  if (index < 0)
    return Value{};
  return at(index);
}

/*!
 * \fn Engine* engine() const
 * \brief Returns the script engine.
 *
 */
Engine* Object::engine() const
{
  return d.engine();
}

/*!
 * \fn bool operator==(const Object& other) const
 * \brief tests if two objects are the same object
 *
 * This function does not compare based on the object value but rather 
 * the object address.
 */
bool Object::operator==(const Object & other) const
{
  return other.d == d;
}

} // namespace script
