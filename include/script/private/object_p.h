// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_OBJECT_P_H
#define LIBSCRIPT_OBJECT_P_H

#include "script/class.h"
#include "script/userdata.h"

namespace script
{

class ObjectImpl
{
public:
  Class instanceOf;
  std::vector<Value> attributes;
  std::shared_ptr<UserData> data;

  ObjectImpl(const Class & c)
    : instanceOf(c)
  {
    attributes.reserve(c.cumulatedDataMemberCount());
  }
};

} // namespace script

#endif // LIBSCRIPT_OBJECT_P_H
