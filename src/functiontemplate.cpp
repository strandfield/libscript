// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/functiontemplate.h"
#include "script/private/template_p.h"

#include "script/private/function_p.h"
#include "script/private/programfunction.h"
#include "script/function-blueprint.h"
#include "script/functiontemplatenativebackend.h"
#include "script/functiontemplateprocessor.h"
#include "script/templateargumentdeduction.h"

#include "script/compiler/compiler.h"
#include "script/compiler/functionprocessor.h"

namespace script
{

FunctionTemplate FunctionTemplateNativeBackend::functionTemplate() const
{
  return FunctionTemplate{ m_template.lock() };
}

void ScriptFunctionTemplateBackend::deduce(TemplateArgumentDeduction& deduction, const std::vector<TemplateArgument>& targs, const std::vector<Type>& itypes) 
{
  deduction.fill(functionTemplate(), targs, itypes, this->definition.decl_);
}

void ScriptFunctionTemplateBackend::substitute(FunctionBlueprint& blueprint, const std::vector<TemplateArgument>& targs)
{
  compiler::FunctionProcessor fp{ blueprint.engine()->compiler() };

  auto fundecl = std::static_pointer_cast<ast::FunctionDecl>(this->definition.decl_->declaration);

  auto tparamscope = functionTemplate().argumentScope(targs);
  fp.generic_fill(blueprint, fundecl, tparamscope);
}

std::pair<NativeFunctionSignature, std::shared_ptr<UserData>> ScriptFunctionTemplateBackend::instantiate(Function& function)
{
  Engine* e = functionTemplate().engine();
  compiler::Compiler* compiler = e->compiler();
  auto decl = std::static_pointer_cast<ast::FunctionDecl>(this->definition.decl_->declaration);
  compiler->instantiate(decl, function, functionTemplate().argumentScope(function.arguments()));

  return { nullptr, nullptr };
}

FunctionTemplate::FunctionTemplate(const std::shared_ptr<FunctionTemplateImpl> & impl)
  : Template(impl)
{

}

FunctionTemplateNativeBackend* FunctionTemplate::backend() const
{
  return impl()->backend.get();
}

bool FunctionTemplate::hasInstance(const std::vector<TemplateArgument> & args, Function *value) const
{
  auto d = impl();
  auto it = d->instances.find(args);
  if (it == d->instances.end())
    return false;
  if (value != nullptr)
    *value = it->second;
  return true;
}

Function FunctionTemplate::getInstance(const std::vector<TemplateArgument> & args)
{
  Function ret;
  if (hasInstance(args, &ret))
    return ret;

  auto d = impl();

  FunctionTemplateProcessor ftp;
  ret = ftp.deduce_substitute(*this, args, {});
  ftp.instantiate(ret);

  d->instances[args] = ret;
  return ret;
}

Function FunctionTemplate::addSpecialization(const std::vector<TemplateArgument> & args, const FunctionBlueprint& opts)
{
  auto d = impl();
  auto impl = FunctionTemplateInstance::create(*this, args, opts);
  Function ret{ impl };
  if (ret.isNull())
    return ret;
  d->instances[args] = ret;
  return ret;
}

const std::map<std::vector<TemplateArgument>, Function, TemplateArgumentComparison> & FunctionTemplate::instances() const
{
  auto d = impl();
  return d->instances;
}

std::shared_ptr<FunctionTemplateImpl> FunctionTemplate::impl() const
{
  return std::dynamic_pointer_cast<FunctionTemplateImpl>(d);
}

} // namespace script
