// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/classtemplate.h"
#include "script/private/template_p.h"

#include "script/class.h"
#include "script/classtemplatespecializationbuilder.h"
#include "script/templateargumentprocessor.h"

#include "script/private/templateargumentscope_p.h"

namespace script
{

ClassTemplate::ClassTemplate(const std::shared_ptr<ClassTemplateImpl> & impl)
  : Template(impl)
{

}

ClassTemplateNativeBackend* ClassTemplate::backend() const
{
  return impl()->backend.get();
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
  /// TODO: should we check if the list of args is complete
  // we could use default template argument value

  Class ret;
  if (hasInstance(args, &ret))
    return ret;

  auto d = impl();

  TemplateArgumentProcessor tnp;
  ret = tnp.instantiate(*this, args);

  /// TODO : this might be unnecessary
  d->instances[args] = ret;

  return ret;
}

ClassTemplateSpecializationBuilder ClassTemplate::Specialization(const std::vector<TemplateArgument> & args)
{
  auto targs = args;
  return ClassTemplateSpecializationBuilder{ *this, std::move(targs) };
}

ClassTemplateSpecializationBuilder ClassTemplate::Specialization(std::vector<TemplateArgument> && args)
{
  return ClassTemplateSpecializationBuilder{ *this, std::move(args) };
}

const std::vector<PartialTemplateSpecialization> & ClassTemplate::partialSpecializations() const
{
  return impl()->specializations();
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



PartialTemplateSpecialization::PartialTemplateSpecialization(const std::shared_ptr<PartialTemplateSpecializationImpl> & impl)
  : d(impl)
{

}

bool PartialTemplateSpecialization::isNull() const
{
  return d == nullptr;
}

const std::vector<TemplateParameter>& PartialTemplateSpecialization::parameters() const
{
  return d->parameters;
}

Scope PartialTemplateSpecialization::scope() const
{
  return d->scope;
}

Scope PartialTemplateSpecialization::argumentScope(const std::vector<TemplateArgument>& args) const
{
  auto ret = std::make_shared<TemplateArgumentScope>(Template(d), args);
  ret->parent = d->scope.impl();
  return Scope{ ret };
}

Scope PartialTemplateSpecialization::parameterScope() const
{
  auto tparamscope = std::make_shared<TemplateParameterScope>(Template(d));
  tparamscope->parent = scope().impl();
  return Scope{ tparamscope };
}

const std::vector<std::shared_ptr<ast::Node>> & PartialTemplateSpecialization::arguments() const
{
  return impl()->definition.get_class_decl()->name->as<ast::TemplateIdentifier>().arguments;
}

ClassTemplate PartialTemplateSpecialization::specializationOf() const
{
  return ClassTemplate{ impl()->class_template.lock() };
}

std::shared_ptr<PartialTemplateSpecializationImpl> PartialTemplateSpecialization::impl() const
{
  return std::static_pointer_cast<PartialTemplateSpecializationImpl>(d);
}

bool operator==(const PartialTemplateSpecialization& lhs, const PartialTemplateSpecialization& rhs)
{
  return lhs.impl() == rhs.impl();
}

} // namespace script
