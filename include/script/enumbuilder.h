
// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_ENUM_BUILDER_H
#define LIBSCRIPT_ENUM_BUILDER_H

#include "script/symbol.h"

#include <stdexcept>

namespace script
{

class Enum;

class LIBSCRIPT_API EnumBuilder
{
public:
  Symbol symbol;
  std::string name;
  bool is_enum_class;
  int id;

public:
  EnumBuilder(const EnumBuilder & ) = default;
  ~EnumBuilder() = default;

  EnumBuilder(const Symbol & s, const std::string & n)
    : symbol(s), name(n), is_enum_class(false), id(0) { }
  EnumBuilder(const Symbol & s, std::string && n)
    : symbol(s), name(std::move(n)), is_enum_class(false), id(0) { }

  EnumBuilder & setEnumClass(bool on = true) {
    is_enum_class = on;
    return (*this);
  }

  EnumBuilder & setId(int n) 
  {
    id = n;

    if ((Type::FirstEnumType & 0xFFFF) > (id & 0xFFFF) || (Type::LastEnumType & 0xFFFF) < (id & 0xFFFF))
    {
      throw std::runtime_error{ "Bad id" };
    }

    return (*this);
  }

  Enum get();
  void create();
};

} // namespace script

#endif // LIBSCRIPT_ENUM_BUILDER_H
