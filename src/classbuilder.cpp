// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/classbuilder.h"

#include "script/class.h"
#include "script/classtemplate.h"
#include "script/engine.h"

#include "script/private/class_p.h"
#include "script/private/engine_p.h"
#include "script/private/namespace_p.h"
#include "script/private/template_p.h"

namespace script
{

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

static void fill_class(std::shared_ptr<ClassImpl> impl, const ClassBuilder & opts)
{
  impl->set_parent(impl->engine->getClass(opts.base));
  impl->dataMembers = opts.dataMembers;
  impl->isFinal = opts.isFinal;
  impl->data = opts.userdata;
  impl->enclosing_symbol = opts.symbol.impl();
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

Class ClassBuilder::get(const ClassTemplate & ct, std::vector<TemplateArgument> && targs)
{
  auto ret = std::make_shared<ClassTemplateInstance>(ct, std::move(targs), -1, std::move(name), symbol.engine());

  fill_class(ret, *this);

  symbol.engine()->implementation()->register_class(Class{ ret }, id);

  ct.impl()->instances[ret->template_arguments] = Class{ ret };

  return Class{ ret };
}

void ClassBuilder::create()
{
  get();
}

} // namespace script
