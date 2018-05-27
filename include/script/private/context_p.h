// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_CONTEXT_P_H
#define LIBSCRIPT_CONTEXT_P_H

#include <map>

#include "script/value.h"

namespace script
{

class ContextImpl
{
public:
  Engine *engine;
  int id;
  std::string name;
  std::map<std::string, Value> variables;

  ContextImpl(Engine *e, int i, const std::string & n)
    : engine(e)
    , id(i)
    , name(n)
  {

  }
};

} // namespace script

#endif // LIBSCRIPT_CONTEXT_P_H
