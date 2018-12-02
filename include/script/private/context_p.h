// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_CONTEXT_P_H
#define LIBSCRIPT_CONTEXT_P_H

#include "script/engine.h"
#include "script/namespace.h"
#include "script/scope.h"
#include "script/value.h"

#include <map>

namespace script
{

class ContextImpl
{
public:
  Engine *engine;
  int id;
  std::string name;
  std::map<std::string, Value> variables;
  Scope scope;


  ContextImpl(Engine *e, int i, const std::string & n)
    : engine(e)
    , id(i)
    , name(n)
    , scope(e->rootNamespace())
  {

  }
};

} // namespace script

#endif // LIBSCRIPT_CONTEXT_P_H
