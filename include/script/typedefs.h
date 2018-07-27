// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TYPEDEFS_H
#define LIBSCRIPT_TYPEDEFS_H

#include "script/symbol.h"
#include "script/types.h"

namespace script
{

class LIBSCRIPT_API Typedef
{
public:
  Typedef() = default;
  Typedef(const Typedef &) = default;
  ~Typedef() = default;

  Typedef(const std::string & name, const Type & t)
    : mName(name)
    , mType(t)
  {

  }

  inline const std::string & name() const { return mName; }
  inline const Type & type() const { return mType; }

  Typedef & operator=(const Typedef &) = default;

private:
  std::string mName;
  Type mType;
};

inline bool operator==(const Typedef & lhs, const Typedef & rhs)
{
  return lhs.type() == rhs.type() && lhs.name() == rhs.name();
}

inline bool operator!=(const Typedef & lhs, const Typedef & rhs)
{
  return !(lhs == rhs);
}


class LIBSCRIPT_API TypedefBuilder
{
public:
  Symbol symbol_;
  std::string name_;
  Type type_;
public:
  TypedefBuilder(const Symbol & s, const std::string & name, const Type & type)
    : symbol_(s), name_(name), type_(type) {}

  TypedefBuilder(const Symbol & s, std::string && name, const Type & type)
    : symbol_(s), name_(std::move(name)), type_(type) {}

  TypedefBuilder(const TypedefBuilder &) = default;
  ~TypedefBuilder() = default;

  void create();
};

} // namespace script

#endif // LIBSCRIPT_TYPEDEFS_H
