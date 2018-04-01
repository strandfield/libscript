// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_SCOPE_P_H
#define LIBSCRIPT_SCOPE_P_H

#include "script/scope.h"

#include "script/class.h"
#include "script/enum.h"
#include "script/namespace.h"
#include "script/script.h"

namespace script
{

class ScopeImpl
{
public:
  std::shared_ptr<ScopeImpl> parent;
  Class theClass;
  Namespace theNamespace;
  Enum theEnum;
  Script theScript;

  ScopeImpl(Enum e, Scope parent);
  ScopeImpl(Class c, Scope parent);
  ScopeImpl(Namespace n, Scope parent);
  ScopeImpl(Script s, Scope parent);

  void addClass(const Class & c);
  void addFunction(const Function & f);
  void addOperator(const Operator & op);
  void add_literal_operator(const LiteralOperator & lo);
  void addCast(const Cast & c);
  void addEnum(const Enum & e);

  void removeClass(const Class & c);
  void removeFunction(const Function & f);
  void removeOperator(const Operator & op);
  void removeCast(const Cast & c);
  void removeEnum(const Enum & e);

};

} // namespace script

#endif // LIBSCRIPT_SCOPE_P_H
