// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_ENUM_P_H
#define LIBSCRIPT_ENUM_P_H

#include "script/private/script_p.h"

#include "script/operator.h"

namespace script
{

class EnumImpl
{
public:
  Engine *engine;
  int id;
  std::string name;
  bool enumClass;
  std::map<std::string, int> values;
  Operator assignment;
  std::weak_ptr<SymbolImpl> enclosing_symbol;

  EnumImpl(int i, const std::string & n, Engine *e);
};

} // namespace script

#endif // LIBSCRIPT_ENUM_P_H
