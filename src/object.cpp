// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/object.h"
#include "script/private/object_p.h"

namespace script
{

Object::Object(const std::shared_ptr<ObjectImpl> & impl)
  : d(impl)
{

}

bool Object::isNull() const
{
  return d == nullptr;
}

Class Object::instanceOf() const
{
  return d->instanceOf;
}

int Object::attributeCount() const
{
  return d->attributes.size();
}

Value Object::getAttribute(int index) const
{
  return d->attributes.at(index);
}

Value Object::getAttribute(const std::string & attrName) const
{
  Class c = instanceOf();
  int index = c.attributeIndex(attrName);
  if (index < 0)
    return Value{};
  return d->attributes[index];
}

Engine* Object::engine() const
{
  return d->instanceOf.engine();
}

ObjectImpl * Object::implementation() const
{
  return d.get();
}

std::weak_ptr<ObjectImpl> Object::weakref() const
{
  return std::weak_ptr<ObjectImpl>{d};
}

bool Object::operator==(const Object & other) const
{
  return other.d == d;
}

} // namespace script
