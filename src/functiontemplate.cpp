// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/functiontemplate.h"
#include "script/private/template_p.h"

#include "script/private/function_p.h"
#include "script/functionbuilder.h"
#include "script/functiontemplateprocessor.h"

namespace script
{

FunctionTemplate::FunctionTemplate(const std::shared_ptr<FunctionTemplateImpl> & impl)
  : Template(impl)
{

}

bool FunctionTemplate::is_native() const
{
  auto d = impl();
  return d->callbacks.deduction != nullptr && d->callbacks.substitution != nullptr
    && d->callbacks.instantiation != nullptr;
}

const FunctionTemplateCallbacks & FunctionTemplate::native_callbacks() const
{
  return impl()->callbacks;
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

Function FunctionTemplate::addSpecialization(const std::vector<TemplateArgument> & args, const FunctionBuilder & opts)
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
