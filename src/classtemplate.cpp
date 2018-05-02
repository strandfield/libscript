// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/classtemplate.h"
#include "template_p.h"

#include "script/class.h"
#include "class_p.h"
#include "script/engine.h"
#include "engine_p.h"

namespace script
{

ClassTemplate::ClassTemplate(const std::shared_ptr<ClassTemplateImpl> & impl)
  : Template(impl)
{

}

bool ClassTemplate::hasInstance(const std::vector<TemplateArgument> & args, Class *value) const
{
  auto d = impl();
  auto it = d->instances.find(args);
  if (it == d->instances.end())
    return false;
  if (value != nullptr)
    *value = it->second;
  return true;
}

Class ClassTemplate::getInstance(const std::vector<TemplateArgument> & args)
{
  Class ret;
  if (hasInstance(args, &ret))
    return ret;

  auto d = impl();
  ret = d->instantiate(*this, args);
  if (ret.isNull())
    throw TemplateInstantiationError{ std::string{ "An error occurred while instantiating the '" }
  +d->name + std::string{ "' class template" } };

  d->instances[args] = ret;
  return ret;
}

Class ClassTemplate::addSpecialization(const std::vector<TemplateArgument> & args, const ClassBuilder & opts)
{
  auto d = impl();
  Class ret = build(opts, args);
  d->instances[args] = ret;
  return ret;
}

Class ClassTemplate::build(const ClassBuilder & builder, const std::vector<TemplateArgument> & args) const
{
  auto ret = std::make_shared<ClassTemplateInstance>(*this, args, -1, builder.name, engine());
  ret->set_parent(builder.parent);
  ret->dataMembers = builder.dataMembers;
  ret->isFinal = builder.isFinal;
  ret->data = builder.userdata;
  ret->instance_of = *this;
  ret->template_arguments = args;

  Class result{ ret };
  engine()->implementation()->register_class(result);

  return result;
}

const std::map<std::vector<TemplateArgument>, Class, TemplateArgumentComparison> & ClassTemplate::instances() const
{
  auto d = impl();
  return d->instances;
}

std::shared_ptr<ClassTemplateImpl> ClassTemplate::impl() const
{
  return std::dynamic_pointer_cast<ClassTemplateImpl>(d);
}

} // namespace script
