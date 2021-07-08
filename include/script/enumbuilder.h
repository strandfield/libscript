// Copyright (C) 2018-2021 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_ENUM_BUILDER_H
#define LIBSCRIPT_ENUM_BUILDER_H

#include "script/symbol.h"
#include "script/callbacks.h"

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
  NativeFunctionSignature from_int_callback = nullptr;
  NativeFunctionSignature copy_callback = nullptr;
  NativeFunctionSignature assignment_callback = nullptr;

public:
  EnumBuilder(const EnumBuilder&) = default;
  ~EnumBuilder() = default;

  explicit EnumBuilder(const Symbol& s)
    : symbol(s), is_enum_class(false), id(0)
  {

  }

  EnumBuilder(const Symbol& s, std::string n)
    : symbol(s), name(std::move(n)), is_enum_class(false), id(0) { }

  EnumBuilder & setEnumClass(bool on = true) {
    is_enum_class = on;
    return (*this);
  }

  EnumBuilder & setId(int n) 
  {
    id = n;
    return (*this);
  }

  EnumBuilder& setCallbacks(NativeFunctionSignature from_int, NativeFunctionSignature copy, NativeFunctionSignature assign)
  {
    from_int_callback = from_int;
    copy_callback = copy;
    assignment_callback = assign;
    return *(this);
  }

  EnumBuilder& operator()(std::string n);

  Enum get();
  void create();
};

} // namespace script

#endif // LIBSCRIPT_ENUM_BUILDER_H
