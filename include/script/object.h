// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_OBJECT_H
#define LIBSCRIPT_OBJECT_H

#include "value.h"
#include "class.h"

namespace script
{

class ObjectImpl;

class LIBSCRIPT_API Object
{
public:
  Object() = default;
  Object(const Object & other) = default;
  ~Object() = default;

  Object(const std::shared_ptr<ObjectImpl> & impl);

  bool isNull() const;
  Class instanceOf() const;

  int attributeCount() const;
  Value getAttribute(int index) const;
  Value getAttribute(const std::string & attrName) const;

  Engine* engine() const;

  ObjectImpl * implementation() const;
  std::weak_ptr<ObjectImpl> weakref() const;

  Object & operator=(const Object & other) = default;
  bool operator==(const Object & other) const;
  inline bool operator!=(const Object & other) const { return !operator==(other); }

private:
  std::shared_ptr<ObjectImpl> d;
};

} // namespace script

#endif // LIBSCRIPT_OBJECT_H
