// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/initializerlist.h"

#include "script/engine.h"
#include "script/classtemplateinstancebuilder.h"
#include "script/constructorbuilder.h"
#include "script/destructorbuilder.h"
#include "script/functionbuilder.h"
#include "script/operatorbuilder.h"
#include "script/template.h"
#include "script/templatebuilder.h"
#include "script/typesystem.h"

#include "script/private/array_p.h"
#include "script/private/engine_p.h"
#include "script/private/value_p.h"


namespace script
{

namespace callbacks
{

namespace initializer_list
{

// InitializerList<T>();
Value default_ctor(FunctionCall *c)
{
  Value & self = c->thisObject();
  self.impl()->set_initializer_list(InitializerList{ &self, &self });
  return self;
}

// InitializerList<T>(const InitializerList<T> & other);
Value copy_ctor(FunctionCall *c)
{
  Value & self = c->thisObject();
  InitializerList other = c->arg(1).toInitializerList();
  self.impl()->set_initializer_list(other);
  return self;
}

// ~InitializerList<T>();
Value dtor(FunctionCall *c)
{
  Value & self = c->thisObject();
  self.impl()->clear();
  return self;
}

// int size() const;
Value size(FunctionCall *c)
{
  return c->engine()->newInt(c->arg(0).toInitializerList().size());
}

// iterator begin() const;
Value begin(FunctionCall *c)
{
  InitializerList self = c->arg(0).toInitializerList();

  Value ret = c->engine()->construct(c->callee().returnType(), {});
  ret.impl()->set_initializer_list(InitializerList{ self.begin(), self.begin() });
  return ret;
}

// iterator end() const;
Value end(FunctionCall *c)
{
  InitializerList self = c->arg(0).toInitializerList();

  Value ret = c->engine()->construct(c->callee().returnType(), {});
  ret.impl()->set_initializer_list(InitializerList{ self.end(), self.end() });
  return ret;
}

// InitializerList<T> & operator=(const InitializerList<T> & other);
Value assign(FunctionCall *c)
{
  Value & self = c->thisObject();
  InitializerList other = c->arg(1).toInitializerList();
  self.impl()->set_initializer_list(other);
  return self;
}

namespace iterator
{

// iterator();
Value default_ctor(FunctionCall *c)
{
  return c->thisObject();
}

// iterator(const iterator & other);
Value copy_ctor(FunctionCall *c)
{
  Value & self = c->thisObject();
  self.impl()->set_initializer_list(c->arg(1).impl()->get_initializer_list());
  return self;
}

// ~iterator();
Value dtor(FunctionCall *c)
{
  Value & self = c->thisObject();
  self.impl()->set_initializer_list(InitializerList{ nullptr, nullptr });
  return self;
}

// const T & get() const;
Value get(FunctionCall *c)
{
  Value & self = c->thisObject();
  return *(self.impl()->get_initializer_list().begin());
}

// iterator & operator=(const iterator & other);
Value assign(FunctionCall *c)
{
  Value & self = c->thisObject();
  self.impl()->set_initializer_list(c->arg(1).impl()->get_initializer_list());
  return self;
}

// iterator & operator++()
Value pre_increment(FunctionCall *c)
{
  Value & self = c->thisObject();
  InitializerList iter = self.impl()->get_initializer_list();
  self.impl()->set_initializer_list(InitializerList{ iter.begin() + 1, iter.begin() + 1 });
  return self;
}

// iterator & operator--()
Value pre_decrement(FunctionCall *c)
{
  Value & self = c->thisObject();
  InitializerList iter = self.impl()->get_initializer_list();
  self.impl()->set_initializer_list(InitializerList{ iter.begin() - 1, iter.begin() - 1 });
  return self;
}

// bool operator==(const iterator & other) const;
Value eq(FunctionCall *c)
{
  Value & self = c->thisObject();
  return c->engine()->newBool(self.impl()->get_initializer_list().begin() == c->arg(1).impl()->get_initializer_list().begin());
}

// bool operator!=(const iterator & other) const;
Value neq(FunctionCall *c)
{
  Value & self = c->thisObject();
  return c->engine()->newBool(self.impl()->get_initializer_list().begin() != c->arg(1).impl()->get_initializer_list().begin());
}

} // namespace iterator

} // namespace initializer_list

} // namespace callbacks


static Class register_ilist_iterator(Class & ilist)
{
  Class iter = ilist.newNestedClass("iterator").setFinal(true).get();

  iter.newConstructor(callbacks::initializer_list::iterator::default_ctor).create();
  iter.newConstructor(callbacks::initializer_list::iterator::copy_ctor).params(Type::cref(iter.id())).create();

  iter.newDestructor(callbacks::initializer_list::iterator::dtor).create();

  iter.newMethod("get", callbacks::initializer_list::iterator::get)
    .setConst()
    .returns(Type::cref(ilist.arguments().at(0).type))
    .create();

  iter.newOperator(AssignmentOperator, callbacks::initializer_list::iterator::assign)
    .returns(Type::ref(iter.id()))
    .params(Type::cref(iter.id()))
    .create();

  iter.newOperator(PreIncrementOperator, callbacks::initializer_list::iterator::pre_increment)
    .returns(Type::ref(iter.id()))
    .create();

  iter.newOperator(PreDecrementOperator, callbacks::initializer_list::iterator::pre_decrement)
    .returns(Type::ref(iter.id()))
    .create();

  iter.newOperator(EqualOperator, callbacks::initializer_list::iterator::eq)
    .returns(Type::Boolean)
    .params(Type::cref(iter.id()))
    .create();

  iter.newOperator(InequalOperator, callbacks::initializer_list::iterator::neq)
    .returns(Type::Boolean)
    .params(Type::cref(iter.id()))
    .create();

  return iter;
}


Class InitializerListTemplate::instantiate(ClassTemplateInstanceBuilder& builder)
{
  const auto& arguments = builder.arguments();

  if (arguments.size() != 1)
    throw TemplateInstantiationError{ "Invalid argument count" };

  if (arguments.at(0).kind != TemplateArgument::TypeArgument)
    throw TemplateInstantiationError{ "Argument must be a type" };

  const Type element_type = arguments.at(0).type.baseType();

  Engine * e = builder.getTemplate().engine();

  builder.name = std::string("InitializerList<") + e->typeSystem()->typeName(element_type) + std::string(">");


  Class ilist_class = builder.get();
  const Type ilist_type = ilist_class.id();

  Class iter_type = register_ilist_iterator(ilist_class);

  ilist_class.newConstructor(callbacks::initializer_list::default_ctor).create();

  ilist_class.newConstructor(callbacks::initializer_list::copy_ctor).params(Type::cref(ilist_type)).create();

  ilist_class.newDestructor(callbacks::initializer_list::dtor).create();

  ilist_class.newMethod("size", callbacks::initializer_list::size)
    .setConst().returns(Type::Int).create();

  ilist_class.newMethod("begin", callbacks::initializer_list::begin)
    .setConst().returns(iter_type.id()).create();

  ilist_class.newMethod("end", callbacks::initializer_list::end)
    .setConst().returns(iter_type.id()).create();

  ilist_class.newOperator(AssignmentOperator, callbacks::initializer_list::assign)
    .returns(Type::ref(ilist_type))
    .params(Type::cref(ilist_type)).create();


  return ilist_class;
}

ClassTemplate register_initialize_list_template(Engine *e)
{
  Namespace root = e->rootNamespace();

  std::vector<TemplateParameter> params{
    TemplateParameter{ TemplateParameter::TypeParameter{}, "T" },
  };

  ClassTemplate ilist_template = Symbol{ root }.newClassTemplate("InitializerList")
    .setParams(std::move(params))
    .setScope(Scope{ root })
    .withBackend<InitializerListTemplate>()
    .get();

  return ilist_template;
}

size_t InitializerList::size() const
{ 
  return std::distance(begin_, end_); 
}

} // namespace script
