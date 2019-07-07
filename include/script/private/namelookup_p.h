// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_NAMELOOKUP_P_H
#define LIBSCRIPT_NAMELOOKUP_P_H

#include "script/namelookup.h"

#include "script/class.h"
#include "script/classtemplate.h"
#include "script/enumerator.h"
#include "script/function.h"
#include "script/functiontemplate.h"
#include "script/staticdatamember.h"

namespace script
{

class NameLookupImpl
{
public:
  // user inputs
  Scope scope;
  std::shared_ptr<ast::Identifier> identifier;
  NameLookupOptions options;

  // storing results
  std::vector<Function> functions;
  Type typeResult;
  Value valueResult;
  Class::StaticDataMember staticDataMemberResult;
  Class memberOfResult;
  ClassTemplate classTemplateResult;
  std::vector<FunctionTemplate> functionTemplateResult;
  Scope scopeResult;
  Enumerator enumeratorResult;
  int dataMemberIndex;
  int globalIndex;
  int localIndex;
  int captureIndex;
  int templateParameterIndex;

public:
  NameLookupImpl();
  ~NameLookupImpl();

  Class getClassTemplateInstance(const Scope& scp, ClassTemplate& ct, const std::shared_ptr<ast::TemplateIdentifier>& tmplt);
};

} // namespace script

#endif // LIBSCRIPT_NAMELOOKUP_P_H
