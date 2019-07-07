// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TEMPLATE_P_H
#define LIBSCRIPT_TEMPLATE_P_H

#include "script/classtemplate.h" // for PartialTemplateSpecialization, try remove
#include "script/scope.h"
#include "script/symbol.h"
#include "script/templateparameter.h"
#include "script/classtemplatenativebackend.h"
#include "script/functiontemplatenativebackend.h"

#include "script/compiler/templatedefinition.h"

#include "script/private/symbol_p.h"

#include <map>
#include <vector>

namespace script
{

class SymbolImpl;

class TemplateImpl : public SymbolImpl
{
public:
  TemplateImpl(std::vector<TemplateParameter> && params, const Scope & scp, Engine *e, std::shared_ptr<SymbolImpl> es);
  virtual ~TemplateImpl() {}

  virtual const std::string& name() const = 0;

  std::vector<TemplateParameter> parameters;
  Scope scope;
  Engine *engine;

  Name get_name() const override;
};

class FunctionTemplateImpl : public TemplateImpl
{
public:
  FunctionTemplateImpl(const std::string & n, std::vector<TemplateParameter> && params, const Scope & scp, std::unique_ptr<FunctionTemplateNativeBackend>&& back,
    Engine *e, std::shared_ptr<SymbolImpl> es);
  ~FunctionTemplateImpl();

  std::string function_name;
  std::map<std::vector<TemplateArgument>, Function, TemplateArgumentComparison> instances;
  std::unique_ptr<FunctionTemplateNativeBackend> backend;

  const std::string& name() const override;
};

class ClassTemplateImpl : public TemplateImpl
{
public:
  ClassTemplateImpl(const std::string & n, std::vector<TemplateParameter> && params, const Scope & scp, std::unique_ptr<ClassTemplateNativeBackend>&& back,
    Engine *e, std::shared_ptr<SymbolImpl> es);
  ~ClassTemplateImpl();

  std::string class_name;
  std::map<std::vector<TemplateArgument>, Class, TemplateArgumentComparison> instances;
  std::unique_ptr<ClassTemplateNativeBackend> backend;

  const std::string& name() const override;
  const std::vector<PartialTemplateSpecialization>& specializations() const;
};

class ScriptFunctionTemplateBackend : public FunctionTemplateNativeBackend
{
public:
  compiler::TemplateDefinition definition;

public:
  ScriptFunctionTemplateBackend() = default;
  ~ScriptFunctionTemplateBackend() = default;

  void deduce(TemplateArgumentDeduction& deduction, const std::vector<TemplateArgument>& targs, const std::vector<Type>& itypes) override;
  void substitute(FunctionBuilder& builder, const std::vector<TemplateArgument>& targs) override;
  std::pair<NativeFunctionSignature, std::shared_ptr<UserData>> instantiate(Function& function) override;
};


class ScriptClassTemplateBackend : public ClassTemplateNativeBackend
{
public:
  compiler::TemplateDefinition definition;
  std::vector<PartialTemplateSpecialization> specializations;

  Class instantiate(ClassTemplateInstanceBuilder& builder) override;
};

class PartialTemplateSpecializationImpl : public TemplateImpl
{
public:
  PartialTemplateSpecializationImpl(const ClassTemplate & ct, std::vector<TemplateParameter> && params, const Scope & scp, Engine *e, std::shared_ptr<SymbolImpl> es);
  ~PartialTemplateSpecializationImpl() = default;

  std::weak_ptr<ClassTemplateImpl> class_template;
  compiler::TemplateDefinition definition;

  const std::string& name() const override;
};

} // namespace script

#endif // LIBSCRIPT_TEMPLATE_P_H
