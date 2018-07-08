// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/classtemplate.h"
#include "script/private/template_p.h"

#include "script/class.h"
#include "script/private/class_p.h"
#include "script/engine.h"
#include "script/private/engine_p.h"
#include "script/compiler/templatenameprocessor.h"

namespace script
{

ClassTemplate::ClassTemplate(const std::shared_ptr<ClassTemplateImpl> & impl)
  : Template(impl)
{

}

bool ClassTemplate::is_native() const
{
  return impl()->instantiate != nullptr;
}

NativeClassTemplateInstantiationFunction ClassTemplate::native_callback() const
{
  return impl()->instantiate;
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

  compiler::TemplateNameProcessor tnp;
  ret = tnp.instantiate(*this, args);

  /// TODO : this might be unnecessary
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
  engine()->implementation()->fill_class(ret, builder);

  Class result{ ret };
  engine()->implementation()->register_class(result, builder.id);

  return result;
}

const std::vector<PartialTemplateSpecialization> & ClassTemplate::partialSpecializations() const
{
  return impl()->specializations;
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
  : Template(impl)
{

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


} // namespace script
