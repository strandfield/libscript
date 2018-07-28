// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/initializerlist.h"

#include "script/engine.h"
#include "script/classtemplateinstancebuilder.h"
#include "script/functionbuilder.h"
#include "script/template.h"
#include "script/templatebuilder.h"

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
  InitializerList other = c->arg(0).toInitializerList();
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
  return c->engine()->newInt(c->thisObject().toInitializerList().size());
}

// iterator begin() const;
Value begin(FunctionCall *c)
{
  InitializerList self = c->thisObject().toInitializerList();

  Value ret = c->engine()->construct(c->callee().returnType(), {});
  ret.impl()->data.builtin.valueptr = self.begin();
  return ret;
}

// iterator end() const;
Value end(FunctionCall *c)
{
  InitializerList self = c->thisObject().toInitializerList();

  Value ret = c->engine()->construct(c->callee().returnType(), {});
  ret.impl()->data.builtin.valueptr = self.end();
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
  Value *other = c->arg(0).impl()->data.builtin.valueptr;
  self.impl()->data.builtin.valueptr = other;
  return self;
}

// ~iterator();
Value dtor(FunctionCall *c)
{
  Value & self = c->thisObject();
  self.impl()->data.builtin.valueptr = nullptr;
  return self;
}

// const T & get() const;
Value get(FunctionCall *c)
{
  Value & self = c->thisObject();
  return *(self.impl()->data.builtin.valueptr);
}

// iterator & operator=(const iterator & other);
Value assign(FunctionCall *c)
{
  Value & self = c->thisObject();
  Value *other = c->arg(1).impl()->data.builtin.valueptr;
  self.impl()->data.builtin.valueptr = other;
  return self;
}

// iterator & operator++()
Value pre_increment(FunctionCall *c)
{
  Value & self = c->thisObject();
  (self.impl()->data.builtin.valueptr)++;
  return self;
}

// iterator & operator--()
Value pre_decrement(FunctionCall *c)
{
  Value & self = c->thisObject();
  (self.impl()->data.builtin.valueptr)--;
  return self;
}

// bool operator==(const iterator & other) const;
Value eq(FunctionCall *c)
{
  Value & self = c->thisObject();
  Value *other = c->arg(1).impl()->data.builtin.valueptr;
  return c->engine()->newBool(self.impl()->data.builtin.valueptr == other);
}

// bool operator!=(const iterator & other) const;
Value neq(FunctionCall *c)
{
  Value & self = c->thisObject();
  Value *other = c->arg(1).impl()->data.builtin.valueptr;
  return c->engine()->newBool(self.impl()->data.builtin.valueptr != other);
}

} // namespace iterator

} // namespace initializer_list

} // namespace callbacks


static Class register_ilist_iterator(Class & ilist)
{
  Class iter = ilist.NestedClass("iterator").setFinal(true).get();

  iter.Constructor(callbacks::initializer_list::iterator::default_ctor).create();
  iter.Constructor(callbacks::initializer_list::iterator::copy_ctor).params(Type::cref(iter.id())).create();

  iter.newDestructor(callbacks::initializer_list::iterator::dtor);

  iter.Method("get", callbacks::initializer_list::iterator::get)
    .setConst()
    .returns(Type::cref(ilist.arguments().at(0).type))
    .create();

  iter.Operation(AssignmentOperator, callbacks::initializer_list::iterator::assign)
    .returns(Type::ref(iter.id()))
    .params(Type::cref(iter.id()))
    .create();

  iter.Operation(PreIncrementOperator, callbacks::initializer_list::iterator::pre_increment)
    .returns(Type::ref(iter.id()))
    .create();

  iter.Operation(PreDecrementOperator, callbacks::initializer_list::iterator::pre_decrement)
    .returns(Type::ref(iter.id()))
    .create();

  iter.Operation(EqualOperator, callbacks::initializer_list::iterator::eq)
    .returns(Type::Boolean)
    .params(Type::cref(iter.id()))
    .create();

  iter.Operation(InequalOperator, callbacks::initializer_list::iterator::neq)
    .returns(Type::Boolean)
    .params(Type::cref(iter.id()))
    .create();

  return iter;
}


Class instantiate_initializer_list_class(ClassTemplateInstanceBuilder & builder)
{
  const auto & arguments = builder.arguments();

  if (arguments.size() != 1)
    throw TemplateInstantiationError{"Invalid argument count"};

  if (arguments.at(0).kind != TemplateArgument::TypeArgument)
    throw TemplateInstantiationError{ "Argument must be a type" };

  const Type element_type = arguments.at(0).type.baseType();

  Engine *e = builder.getTemplate().engine();
 
  builder.name = std::string("InitializerList<") + e->typeName(element_type) + std::string(">");


  Class ilist_class = builder.get();
  const Type ilist_type = ilist_class.id();

  Class iter_type = register_ilist_iterator(ilist_class);
  
  ilist_class.Constructor(callbacks::initializer_list::default_ctor).create();

  ilist_class.Constructor(callbacks::initializer_list::copy_ctor).params(Type::cref(ilist_type)).create();

  ilist_class.newDestructor(callbacks::initializer_list::dtor);

  ilist_class.Method("size", callbacks::initializer_list::size)
    .setConst().returns(Type::Int).create();

  ilist_class.Method("begin", callbacks::initializer_list::begin)
    .setConst().returns(iter_type.id()).create();

  ilist_class.Method("end", callbacks::initializer_list::end)
    .setConst().returns(iter_type.id()).create();

  ilist_class.Operation(AssignmentOperator, callbacks::initializer_list::assign)
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

  ClassTemplate ilist_template = Symbol{ root }.ClassTemplate("InitializerList")
    .setParams(std::move(params))
    .setScope(Scope{ root })
    .setCallback(instantiate_initializer_list_class)
    .get();

  return ilist_template;
}

size_t InitializerList::size() const
{ 
  return std::distance(begin_, end_); 
}

} // namespace script