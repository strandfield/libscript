// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TEMPLATE_P_H
#define LIBSCRIPT_TEMPLATE_P_H

#include "script/template.h"

namespace script
{

class ScriptImpl;

class TemplateImpl
{
public:
  TemplateImpl(const std::string & n, NativeTemplateDeductionFunction deduc, Engine *e, std::shared_ptr<ScriptImpl> s);
  virtual ~TemplateImpl() {}

  std::string name;
  NativeTemplateDeductionFunction deduction;
  std::weak_ptr<ScriptImpl> script;
  Engine *engine;
};

class FunctionTemplateImpl : public TemplateImpl
{
public:
  FunctionTemplateImpl(const std::string & n, NativeTemplateDeductionFunction deduc,
    NativeFunctionTemplateInstantiationFunction inst,
    Engine *e, std::shared_ptr<ScriptImpl> s);
  ~FunctionTemplateImpl();

  std::map<std::vector<TemplateArgument>, Function, TemplateArgumentComparison> instances;
  NativeFunctionTemplateInstantiationFunction instantiate;
};

class ClassTemplateImpl : public TemplateImpl
{
public:
  ClassTemplateImpl(const std::string & n, NativeClassTemplateInstantiationFunction inst,
    Engine *e, std::shared_ptr<ScriptImpl> s);
  ~ClassTemplateImpl();

  std::map<std::vector<TemplateArgument>, Class, TemplateArgumentComparison> instances;
  NativeClassTemplateInstantiationFunction instantiate;
};

} // namespace script

#endif // LIBSCRIPT_TEMPLATE_P_H
