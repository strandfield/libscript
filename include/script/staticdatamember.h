// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_CLASS_STATIC_DATA_MEMBER_H
#define LIBSCRIPT_CLASS_STATIC_DATA_MEMBER_H

#include "script/accessspecifier.h"
#include "script/value.h"

namespace script
{

class LIBSCRIPT_API StaticDataMember
{
public:
  std::string name;
  Value value;

  StaticDataMember() = default;
  StaticDataMember(const StaticDataMember &) = default;
  StaticDataMember(const std::string &n, const Value & val, AccessSpecifier aspec = AccessSpecifier::Public);
  ~StaticDataMember() = default;

  StaticDataMember & operator=(const StaticDataMember &) = default;

  inline bool isNull() const { return this->value.isNull(); }
  AccessSpecifier accessibility() const;
};
} // namespace script

#endif // LIBSCRIPT_CLASS_STATIC_DATA_MEMBER_H
