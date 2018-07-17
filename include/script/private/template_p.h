// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TEMPLATE_P_H
#define LIBSCRIPT_TEMPLATE_P_H

#include "script/classtemplate.h" // for PartialTemplateSpecialization, try remove
#include "script/scope.h"
#include "script/symbol.h"
#include "script/templatecallbacks.h"
#include "script/templateparameter.h"

#include "script/compiler/templatedefinition.h"

#include <map>
#include <vector>

namespace script
{

class SymbolImpl;

class TemplateImpl
{
public:
  TemplateImpl(const std::string & n, std::vector<TemplateParameter> && params, const Scope & scp, Engine *e, std::shared_ptr<SymbolImpl> es);
  virtual ~TemplateImpl() {}

  std::string name;
  std::vector<TemplateParameter> parameters;
  Scope scope;
  std::weak_ptr<SymbolImpl> enclosing_symbol;
  Engine *engine;
  compiler::TemplateDefinition definition;
};

class FunctionTemplateImpl : public TemplateImpl
{
public:
  FunctionTemplateImpl(const std::string & n, std::vector<TemplateParameter> && params, const Scope & scp, NativeFunctionTemplateDeductionCallback deduc,
    NativeFunctionTemplateSubstitutionCallback substitute, NativeFunctionTemplateInstantiationCallback callback,
    Engine *e, std::shared_ptr<SymbolImpl> es);
  ~FunctionTemplateImpl();

  std::map<std::vector<TemplateArgument>, Function, TemplateArgumentComparison> instances;
  FunctionTemplateCallbacks callbacks;
};

class ClassTemplateImpl : public TemplateImpl
{
public:
  ClassTemplateImpl(const std::string & n, std::vector<TemplateParameter> && params, const Scope & scp, NativeClassTemplateInstantiationFunction inst,
    Engine *e, std::shared_ptr<SymbolImpl> es);
  ~ClassTemplateImpl();

  std::map<std::vector<TemplateArgument>, Class, TemplateArgumentComparison> instances;
  NativeClassTemplateInstantiationFunction instantiate;
  std::vector<PartialTemplateSpecialization> specializations;
};

class PartialTemplateSpecializationImpl : public TemplateImpl
{
public:
  PartialTemplateSpecializationImpl(const ClassTemplate & ct, std::vector<TemplateParameter> && params, const Scope & scp, Engine *e, std::shared_ptr<SymbolImpl> es);
  ~PartialTemplateSpecializationImpl() = default;

  std::weak_ptr<ClassTemplateImpl> class_template;
};

} // namespace script

#endif // LIBSCRIPT_TEMPLATE_P_H
