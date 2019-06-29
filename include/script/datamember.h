// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_CLASS_DATA_MEMBER_H
#define LIBSCRIPT_CLASS_DATA_MEMBER_H

#include "script/accessspecifier.h"
#include "script/types.h"

#include <string>

namespace script
{

class LIBSCRIPT_API DataMember
{
public:
  Type type;
  std::string name;

  DataMember() = default;
  DataMember(const DataMember &) = default;
  DataMember(const Type & t, const std::string & name, AccessSpecifier aspec = AccessSpecifier::Public);
  ~DataMember() = default;

  DataMember & operator=(const DataMember &) = default;

  inline bool isNull() const { return name.empty(); }
  AccessSpecifier accessibility() const;
};

} // namespace script

#endif // LIBSCRIPT_CLASS_DATA_MEMBER_H
