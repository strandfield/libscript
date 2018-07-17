// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_OBJECT_H
#define LIBSCRIPT_OBJECT_H

#include "script/value.h"
#include "script/class.h" /// TODO: replace by forward declaration

namespace script
{

class ObjectImpl;

class LIBSCRIPT_API Object
{
public:
  Object() = default;
  Object(const Object & other) = default;
  ~Object() = default;

  explicit Object(const std::shared_ptr<ObjectImpl> & impl);

  bool isNull() const;
  Class instanceOf() const;

  int attributeCount() const;
  Value getAttribute(int index) const;
  Value getAttribute(const std::string & attrName) const;

  Engine* engine() const;

  inline const std::shared_ptr<ObjectImpl> & impl() const { return d; }

  Object & operator=(const Object & other) = default;
  bool operator==(const Object & other) const;
  inline bool operator!=(const Object & other) const { return !operator==(other); }

private:
  std::shared_ptr<ObjectImpl> d;
};

} // namespace script

#endif // LIBSCRIPT_OBJECT_H
