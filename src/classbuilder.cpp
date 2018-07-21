// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/classbuilder.h"
#include "script/classtemplateinstancebuilder.h"
#include "script/classtemplatespecializationbuilder.h"

#include "script/class.h"
#include "script/classtemplate.h"
#include "script/engine.h"

#include "script/private/class_p.h"
#include "script/private/engine_p.h"
#include "script/private/namespace_p.h"
#include "script/private/template_p.h"

namespace script
{

template<typename Derived>
static void fill_class(std::shared_ptr<ClassImpl> impl, const ClassBuilderBase<Derived> & opts)
{
  impl->set_parent(impl->engine->getClass(opts.base));
  impl->dataMembers = opts.dataMembers;
  impl->isFinal = opts.isFinal;
  impl->data = opts.userdata;
  impl->enclosing_symbol = opts.symbol.impl();
}

ClassBuilder::ClassBuilder(const Symbol &s, const std::string & n)
  : Base(s, n)
{

}

ClassBuilder::ClassBuilder(const Symbol &s, std::string && n)
  : Base(s, std::move(n))
{

}

ClassBuilder & ClassBuilder::setParent(const Class & p)
{
  return setBase(p);
}
ClassBuilder & ClassBuilder::setBase(const Class & b)
{
  base = b.id();
  return (*this);
}

Class ClassBuilder::get()
{
  Class ret{ std::make_shared<ClassImpl>(-1, std::move(name), symbol.engine()) };

  fill_class(ret.impl(), *this);

  symbol.engine()->implementation()->register_class(ret, id);

  if (symbol.isClass())
    std::static_pointer_cast<ClassImpl>(symbol.impl())->classes.push_back(ret);
  else
    std::static_pointer_cast<NamespaceImpl>(symbol.impl())->classes.push_back(ret);

  return ret;
}

void ClassBuilder::create()
{
  get();
}


ClassTemplateInstanceBuilder::ClassTemplateInstanceBuilder(const ClassTemplate & t, std::vector<TemplateArgument> && targs)
  : Base(t.enclosingSymbol(), std::string{})
  , template_(t)
  ,  arguments_(std::move(targs))
{

}

Class ClassTemplateInstanceBuilder::get()
{
  auto ret = std::make_shared<ClassTemplateInstance>(template_, std::move(arguments_), -1, std::move(name), template_.engine());
  fill_class(ret, *this);

  ret->enclosing_symbol = symbol.impl();

  Class class_result{ ret };
  template_.engine()->implementation()->register_class(class_result);

  return class_result;
}


ClassTemplateSpecializationBuilder::ClassTemplateSpecializationBuilder(const ClassTemplate & t, std::vector<TemplateArgument> && targs)
  : Base(t.enclosingSymbol(), std::string{})
  , template_(t)
  , arguments_(std::move(targs))
{

}

Class ClassTemplateSpecializationBuilder::get()
{
  auto ret = std::make_shared<ClassTemplateInstance>(template_, std::move(arguments_), -1, std::move(name), template_.engine());
  fill_class(ret, *this);

  ret->enclosing_symbol = symbol.impl();

  Class class_result{ ret };
  template_.engine()->implementation()->register_class(class_result);

  template_.impl()->instances[class_result.arguments()] = class_result;

  return class_result;
}

} // namespace script
