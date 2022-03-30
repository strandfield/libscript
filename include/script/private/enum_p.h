// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_ENUM_P_H
#define LIBSCRIPT_ENUM_P_H

#include "script/private/symbol_p.h"

#include "script/operator.h"

#include <map>

namespace script
{

class EnumImpl // @TODO: pubic SymbolImpl
{
public:
  Engine *engine;
  int id;
  std::string name;
  bool enumClass;
  std::map<std::string, int> values;
  // @TODO: replace by virtual functions
  // virtual script::Value from_int(int n);
  // virtual script::Value copy(const script::Value& other);
  // virtual void assign(script::Value& lhs, const script::Value& rhs);
  Function from_int;
  Function copy;
  Operator assignment;
  std::weak_ptr<SymbolImpl> enclosing_symbol;

  EnumImpl(int i, const std::string & n, Engine *e);
};

} // namespace script

#endif // LIBSCRIPT_ENUM_P_H
