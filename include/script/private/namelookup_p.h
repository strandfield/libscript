// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_NAMELOOKUP_P_H
#define LIBSCRIPT_NAMELOOKUP_P_H

#include "script/classtemplate.h"
#include "script/enumvalue.h"
#include "script/function.h"
#include "script/functiontemplate.h"

#include "script/compiler/templatenameprocessor.h"

namespace script
{

class Class;

class NameLookupImpl
{
public:
  // user inputs
  Scope scope;
  std::shared_ptr<ast::Identifier> identifier;
  // template-related stuff
  compiler::TemplateNameProcessor default_template_;
  compiler::TemplateNameProcessor *template_;

  // storing results
  std::vector<Function> functions;
  Type typeResult;
  Value valueResult;
  Class::StaticDataMember staticDataMemberResult;
  Class memberOfResult;
  ClassTemplate classTemplateResult;
  std::vector<FunctionTemplate> functionTemplateResult;
  Scope scopeResult;
  EnumValue enumValueResult;
  int dataMemberIndex;
  int globalIndex;
  int localIndex;
  int captureIndex;
  int templateParameterIndex;

public:
  NameLookupImpl();
  ~NameLookupImpl();
};

} // namespace script

#endif // LIBSCRIPT_NAMELOOKUP_P_H
