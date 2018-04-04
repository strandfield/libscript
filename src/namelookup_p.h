// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_NAMELOOKUP_P_H
#define LIBSCRIPT_NAMELOOKUP_P_H

#include "script/function.h"
#include "script/template.h"

namespace script
{

class Class;

class NameLookupImpl
{
public:
  std::string name;
  Scope scope;
  std::vector<Function> functions;
  Type typeResult;
  Value valueResult;
  Template templateResult;
  Namespace namespaceResult;
  EnumValue enumValueResult;
  int dataMemberIndex;
  int globalIndex;
  int localIndex;
  int captureIndex;

public:
  NameLookupImpl();
  NameLookupImpl(const Type & t);
  NameLookupImpl(const Class & cla);
  NameLookupImpl(const Function & f);
};

} // namespace script

#endif // LIBSCRIPT_NAMELOOKUP_P_H
