// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/functiontemplate.h"
#include "template_p.h"

#include "function_p.h"
#include "script/functionbuilder.h"
#include "script/namelookup.h"
#include "namelookup_p.h"
#include "script/templateargumentdeduction.h"

#include "script/program/expression.h"

namespace script
{

FunctionTemplate::FunctionTemplate(const std::shared_ptr<FunctionTemplateImpl> & impl)
  : Template(impl)
{

}

TemplateArgumentDeduction FunctionTemplate::deduce(const std::vector<TemplateArgument> & args, const std::vector<Type> & types) const
{
  return impl()->deduction(*this, args, types);
}

Function FunctionTemplate::substitute(const std::vector<TemplateArgument> & targs) const
{
  return impl()->substitute(*this, targs);
}

void FunctionTemplate::instantiate(Function & f)
{
  if (!f.isTemplateInstance())
    throw std::runtime_error{ "Function is not a template instance" };

  impl()->instantiate(*this, f);

  impl()->instances[f.arguments()] = f;
}

Function FunctionTemplate::build(const FunctionBuilder & builder, const std::vector<TemplateArgument> & args)
{
  auto impl = std::make_shared<FunctionTemplateInstance>(*this, args, builder.name, builder.proto, engine(), builder.flags);
  impl->implementation.callback = builder.callback;
  impl->data = builder.data;
  return Function{ impl };
}

void FunctionTemplate::setInstanceData(Function & f, const std::shared_ptr<UserData> & data)
{
  f.implementation()->data = data;
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
  ret = d->substitute(*this, args);
  ret = d->instantiate(*this, ret);
  if (ret.isNull())
    throw TemplateInstantiationError{ std::string{ "An error occurred while instantiating the '" }
  +d->name + std::string{ "' function template" } };

  d->instances[args] = ret;
  return ret;
}

Function FunctionTemplate::addSpecialization(const std::vector<TemplateArgument> & args, const FunctionBuilder & opts)
{
  auto d = impl();
  Function ret = d->engine->newFunction(opts);
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
