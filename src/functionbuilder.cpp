// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/functionbuilder.h"

#include "script/class.h"
#include "script/private/class_p.h"
#include "script/engine.h"
#include "script/private/cast_p.h"
#include "script/private/function_p.h"
#include "script/private/operator_p.h"
#include "script/private/literals_p.h"
#include "script/namespace.h"
#include "script/private/namespace_p.h"
#include "script/operator.h"

namespace script
{

static void fill(const std::shared_ptr<FunctionImpl> & impl, const FunctionBuilder & opts)
{
  impl->implementation.callback = opts.callback;
  impl->data = opts.data;
  impl->enclosing_symbol = opts.symbol.impl();
}

static Function build_constructor(const FunctionBuilder & opts)
{
  if (!opts.symbol.isClass())
    throw std::runtime_error{ "Cannot create a constructor with invalid class" };

  auto impl = std::make_shared<ConstructorImpl>(opts.proto, opts.engine, opts.flags);
  fill(impl, opts);
  return Function{ impl };
}

static Function build_destructor(const FunctionBuilder & opts)
{
  if (!opts.symbol.isClass())
    throw std::runtime_error{ "Cannot create a constructor with invalid class" };

  auto impl = std::make_shared<DestructorImpl>(opts.proto, opts.engine, opts.flags);
  fill(impl, opts);
  return Function{ impl };
}

static Function build_basic_function(const FunctionBuilder & opts)
{
  auto impl = std::make_shared<RegularFunctionImpl>(opts.name, opts.proto, opts.engine, opts.flags);
  fill(impl, opts);
  return Function{ impl };
}

static Function build_literal_operator(const FunctionBuilder & opts)
{
  auto impl = std::make_shared<LiteralOperatorImpl>(std::string{ opts.name }, opts.proto, opts.engine, opts.flags);
  fill(impl, opts);
  return Function{ impl };
}

static Function build_operator(const FunctionBuilder & opts)
{
  auto impl = std::make_shared<OperatorImpl>(opts.operation, opts.proto, opts.engine, opts.flags);
  fill(impl, opts);
  return Function{ impl };
}

static Function build_cast(const FunctionBuilder & opts)
{
  auto impl = std::make_shared<CastImpl>(opts.proto, opts.engine, opts.flags);
  fill(impl, opts);
  return Function{ impl };
}

static Function build_function(const FunctionBuilder & opts)
{
  switch (opts.kind)
  {
  case Function::Constructor:
    return build_constructor(opts);
  case Function::Destructor:
    return build_destructor(opts);
  case Function::CastFunction:
    return build_cast(opts);
  case Function::StandardFunction:
    return build_basic_function(opts);
  case Function::OperatorFunction:
    return build_operator(opts);
  case Function::LiteralOperatorFunction:
    return build_literal_operator(opts);
  default:
    break;
  }

  throw std::runtime_error{ "FunctionBuilder::get() missing function kind" };
}


FunctionBuilder::FunctionBuilder(Function::Kind k)
  : callback(nullptr)
  , engine(nullptr)
  , kind(k)
  , flags(0)
  , operation(Operator::Null)
{

}

FunctionBuilder::FunctionBuilder(Class cla, Function::Kind k)
  : callback(nullptr)
  , engine(cla.engine())
  , symbol(cla)
  , kind(k)
  , flags(0)
  , operation(Operator::Null)
{
  this->proto.setReturnType(Type::Void);

  if (k != Function::Constructor)
    this->proto.addParameter(Type::ref(cla.id()).withFlag(Type::ThisFlag));
  else 
    this->proto.setReturnType(Type::cref(cla.id()));
}

FunctionBuilder::FunctionBuilder(Class cla, OperatorName op)
  : callback(nullptr)
  , engine(cla.engine())
  , symbol(cla)
  , kind(Function::OperatorFunction)
  , flags(0)
  , operation(op)
{
  this->proto.setReturnType(Type::Void);
  this->proto.addParameter(Type::ref(cla.id()).withFlag(Type::ThisFlag));
}

FunctionBuilder::FunctionBuilder(Namespace ns)
  : callback(nullptr)
  , engine(ns.engine())
  , symbol(ns)
  , kind(Function::StandardFunction)
  , flags(0)
  , operation(Operator::Null)
{

}

FunctionBuilder::FunctionBuilder(Namespace ns, OperatorName op)
  : callback(nullptr)
  , engine(ns.engine())
  , symbol(ns)
  , kind(Function::OperatorFunction)
  , flags(0)
  , operation(op)
{

}

FunctionBuilder::FunctionBuilder(Namespace ns, LiteralOperatorTag, const std::string & suffix)
  : callback(nullptr)
  , engine(ns.engine())
  , symbol(ns)
  , kind(Function::LiteralOperatorFunction)
  , flags(0)
  , name(suffix)
  , operation(Operator::Null)
{

}

FunctionBuilder FunctionBuilder::Destructor(const Class & cla, NativeFunctionSignature impl)
{
  FunctionBuilder ret{ Function::Destructor };
  ret.callback = impl;
  ret.symbol = Symbol{ cla };
  ret.proto = Prototype{ Type::Void, Type::cref(cla.id() | Type::ThisFlag) }; /// TODO : not sure about that
  return ret;
}

FunctionBuilder & FunctionBuilder::setConst()
{
  this->proto.setParameter(0, Type::cref(this->proto.at(0)));
  return *(this);
}

FunctionBuilder & FunctionBuilder::setVirtual()
{
  this->flags |= (Function::Virtual << 2);
  return *(this);
}

FunctionBuilder & FunctionBuilder::setPureVirtual()
{
  this->flags |= (Function::Virtual << 2) | (Function::Pure << 2);
  return *(this);
}

FunctionBuilder & FunctionBuilder::setDeleted()
{
  this->flags |= (Function::Delete << 2);
  return *(this);
}

FunctionBuilder & FunctionBuilder::setDefaulted()
{
  this->flags |= (Function::Default << 2);
  return *(this);
}

FunctionBuilder & FunctionBuilder::setConstExpr()
{
  this->flags |= (Function::ConstExpr << 2);
  return *(this);
}

FunctionBuilder & FunctionBuilder::setExplicit()
{
  this->flags |= (Function::Explicit << 2);
  return *(this);
}

FunctionBuilder & FunctionBuilder::setPrototype(const Prototype & proto)
{
  this->proto = proto;
  return *(this);
}

FunctionBuilder & FunctionBuilder::setCallback(NativeFunctionSignature impl)
{
  this->callback = impl;
  return *(this);
}

FunctionBuilder & FunctionBuilder::setData(const std::shared_ptr<UserData> & data)
{
  this->data = data;
  return *(this);
}

FunctionBuilder & FunctionBuilder::setAccessibility(AccessSpecifier aspec)
{
  // erase old access specifier
  this->flags = (this->flags & ~((Function::Private | Function::Protected) << 2));
  int f = 0;
  switch (aspec)
  {
  case script::AccessSpecifier::Public:
    break;
  case script::AccessSpecifier::Protected:
    f = Function::Protected;
    break;
  case script::AccessSpecifier::Private:
    f = Function::Private;
    break;
  }

  this->flags |= (f << 2);
  return (*this);
}

FunctionBuilder & FunctionBuilder::setPublic()
{
  return setAccessibility(AccessSpecifier::Public);
}

FunctionBuilder & FunctionBuilder::setProtected()
{
  return setAccessibility(AccessSpecifier::Protected);
}

FunctionBuilder & FunctionBuilder::setPrivate()
{
  return setAccessibility(AccessSpecifier::Private);
}

FunctionBuilder & FunctionBuilder::setStatic()
{
  flags |= (Function::Static << 2);

  if (proto.count() == 0 || !proto.at(0).testFlag(Type::ThisFlag))
    return *this;

  for (int i(0); i < proto.count() - 1; ++i)
    proto.setParameter(i, proto.at(i+1));
  proto.popParameter();

  return *this;
}

bool FunctionBuilder::isStatic() const
{
  return flags & (Function::Static << 2);
}

FunctionBuilder & FunctionBuilder::setReturnType(const Type & t)
{
  this->proto.setReturnType(t);
  return *(this);
}

FunctionBuilder & FunctionBuilder::addParam(const Type & t)
{
  this->proto.addParameter(t);
  return *(this);
}

script::Function FunctionBuilder::create()
{
  if (this->engine == nullptr)
    this->engine = this->symbol.engine();

  if (this->engine == nullptr)
    throw std::runtime_error{ "FunctionBuilder::create() : null engine" };

  if (is_member_function())
  {
    Class cla = member_of();
    script::Function f = build_function(*this);
    f.impl()->enclosing_symbol = cla.impl();
    if (f.isOperator())
      cla.impl()->operators.push_back(f.toOperator());
    else if (f.isCast())
      cla.impl()->casts.push_back(f.toCast());
    else if (f.isConstructor())
      cla.impl()->registerConstructor(f);
    else if (f.isDestructor())
      cla.impl()->destructor = f;
    else
      cla.impl()->register_function(f);
    return f;
  }
  else if (this->symbol.isNamespace())
  {
    Namespace ns = this->symbol.toNamespace();
    script::Function f = build_function(*this);
    f.impl()->enclosing_symbol = ns.impl();
    if (f.isOperator())
      ns.impl()->operators.push_back(f.toOperator());
    else if (f.isLiteralOperator())
      ns.impl()->literal_operators.push_back(f.toLiteralOperator());
    else
      ns.impl()->functions.push_back(f);
    return f;
  }

  throw std::runtime_error{ "FunctionBuilder::create() : not implemented" };
}

bool FunctionBuilder::is_member_function() const
{
  return this->symbol.isClass() || (this->proto.count() > 0 && this->proto.at(0).testFlag(Type::ThisFlag));
}

Class FunctionBuilder::member_of() const
{
  if (this->symbol.isClass())
    return this->symbol.toClass();

  return this->engine->getClass(this->proto.at(0));
}

} // namespace script
