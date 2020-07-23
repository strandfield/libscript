// Copyright (C) 2018-2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_OBJECT_H
#define LIBSCRIPT_OBJECT_H

#include "script/value.h"

namespace script
{

class Class;

class LIBSCRIPT_API Object
{
public:
  Object() = default;
  Object(const Object &) = default;
  ~Object() = default;

  explicit Object(const Value& val);

  bool isNull() const;
  inline bool isValid() const { return !isNull(); }
  Class instanceOf() const;

  size_t size() const;
  const Value & at(size_t i) const;

  Value get(const std::string & attrName) const;

  Engine* engine() const;

  Object& operator=(const Object &) = default;
  bool operator==(const Object& other) const;

private:
  Value d;
};

inline bool operator!=(const Object& lhs, const Object& rhs) { return !(lhs == rhs); }

} // namespace script

#endif // LIBSCRIPT_OBJECT_H
