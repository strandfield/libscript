// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/object.h"
#include "script/private/object_p.h"

namespace script
{

/*!
 * \class Object
 * \brief Provides a representation of classes instances.
 *
 * The Object class provides a very basic way to represent instances of 
 * a class, i.e. objects.
 * It is basically implemented as a a \t Class and a vector of \t{Value}s (the data-members).
 *
 * This class is the default representation of objects in the library. 
 * This means that it is used to represent objects of user-defined types introduced in 
 * scripts.
 *
 * This class is implicitly shared and has a null-state that can be checked with \m isNull. 
 * Unless stated otherwise, all methods in this class assumes that the object is not null.
 */

/*!
 * \fn Object()
 * \brief Constructs a null object.
 */

/*!
 * \fn Object(const Object & other)
 * \brief Copy constructor.
 *
 * Warning: the class is implicitly shared so this does not copy the data members.
 */


/*!
 * \fn Object(const std::shared_ptr<ObjectImpl> & impl)
 * \brief Constructor used for internal use
 *
 * Constructs an object from its implementation.
 */
Object::Object(const std::shared_ptr<ObjectImpl> & impl)
  : d(impl)
{

}

/*!
 * \fn bool isNull() const
 * \brief Returns whether the object is null.
 *
 */
bool Object::isNull() const
{
  return d == nullptr;
}

/*!
 * \fn Class instanceOf() const
 * \brief Returns the class that was passed upon construction.
 *
 */
Class Object::instanceOf() const
{
  return d->instanceOf;
}

/*!
 * \fn void push(const Value & val)
 * \brief Adds a data-member to the object.
 *
 */
void Object::push(const Value & val)
{
  d->attributes.push_back(val);
}

/*!
 * \fn Value pop()
 * \brief Pops the last data-member inserted.
 *
 */
Value Object::pop()
{
  auto ret = d->attributes.back();
  d->attributes.pop_back();
  return ret;
}

/*!
 * \fn const Value & at(size_t i) const
 * \brief Returns the data-member at the given offset.
 *
 */
const Value & Object::at(size_t i) const
{
  return d->attributes.at(i);
}

/*!
 * \fn size_t size() const
 * \brief Returns the actual number of data-member in the object.
 *
 */
size_t Object::size() const
{
  return d->attributes.size();
}

/*!
 * \fn Value get(const std::string & attrName) const
 * \brief Returns the data-member with the given name.
 *
 * If no such data member exists, this returns a default constructed \t Value.
 */
Value Object::get(const std::string & attrName) const
{
  Class c = instanceOf();
  int index = c.attributeIndex(attrName);
  if (index < 0)
    return Value{};
  return d->attributes[index];
}

/*!
 * \fn Engine* engine() const
 * \brief Returns the script engine.
 *
 */
Engine* Object::engine() const
{
  return d->instanceOf.engine();
}

/*!
 * \fn Object create(const Class & c)
 * \brief Creates a non-null object.
 *
 * This function creates and returns an object with enough space reserved to 
 * store all data-members of the given \t Class.
 * It is your responsability to provides all the data-members by using \m push.
 */
Object Object::create(const Class & c)
{
  return Object{ std::make_shared<ObjectImpl>(c) };
}

/*!
 * \fn bool operator==(const Object & other) const
 * \brief Tests if two objects are the same object.
 *
 * This function does not compare based on the object value but rather 
 * the object address.
 */
bool Object::operator==(const Object & other) const
{
  return other.d == d;
}

} // namespace script
