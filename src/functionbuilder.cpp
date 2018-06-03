// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/functionbuilder.h"

#include "script/class.h"
#include "script/private/class_p.h"
#include "script/engine.h"
#include "script/namespace.h"
#include "script/private/namespace_p.h"

namespace script
{

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
  , special(cla)
  , kind(k)
  , flags(0)
  , operation(Operator::Null)
{
  this->proto.setReturnType(Type::Void);
  this->proto.addArgument(Type::ref(cla.id()).withFlag(Type::ThisFlag));
}

FunctionBuilder::FunctionBuilder(Class cla, Operator::BuiltInOperator op)
  : callback(nullptr)
  , engine(cla.engine())
  , special(cla)
  , kind(Function::OperatorFunction)
  , flags(0)
  , operation(op)
{
  this->proto.setReturnType(Type::Void);
  this->proto.addArgument(Type::ref(cla.id()).withFlag(Type::ThisFlag));
}

FunctionBuilder::FunctionBuilder(Namespace ns)
  : callback(nullptr)
  , engine(ns.engine())
  , namespace_scope(ns)
  , kind(Function::StandardFunction)
  , flags(0)
  , operation(Operator::Null)
{

}

FunctionBuilder::FunctionBuilder(Namespace ns, Operator::BuiltInOperator op)
  : callback(nullptr)
  , engine(ns.engine())
  , namespace_scope(ns)
  , kind(Function::OperatorFunction)
  , flags(0)
  , operation(op)
{

}


FunctionBuilder FunctionBuilder::Function(const std::string & name, const Prototype & proto, NativeFunctionSignature impl)
{
  FunctionBuilder ret{ Function::StandardFunction };
  ret.callback = impl;
  ret.name = name;
  ret.proto = proto;
  return ret;
}

FunctionBuilder FunctionBuilder::Constructor(const Class & cla, Prototype proto, NativeFunctionSignature impl)
{
  FunctionBuilder ret{ Function::Constructor };
  ret.callback = impl;
  ret.special = cla;
  proto.setReturnType(Type::cref(cla.id()));
  ret.proto = std::move(proto);
  return ret;
}

FunctionBuilder FunctionBuilder::Constructor(const Class & cla, NativeFunctionSignature impl)
{
  FunctionBuilder ret{ Function::Constructor };
  ret.callback = impl;
  ret.special = cla;
  ret.proto.setReturnType(Type::cref(cla.id()));
  return ret;
}

FunctionBuilder FunctionBuilder::Destructor(const Class & cla, NativeFunctionSignature impl)
{
  FunctionBuilder ret{ Function::Destructor };
  ret.callback = impl;
  ret.special = cla;
  ret.proto = Prototype{ Type::Void, Type::cref(cla.id() | Type::ThisFlag) }; /// TODO : not sure about that
  return ret;
}

FunctionBuilder FunctionBuilder::Method(const Class & cla, const std::string & name, NativeFunctionSignature impl)
{
  FunctionBuilder ret{ Function::StandardFunction };
  ret.callback = impl;
  ret.name = name;
  ret.proto.setReturnType(Type::Void);
  ret.proto.addArgument(Type::ref(cla.id() | Type::ThisFlag));
  return ret;
}

FunctionBuilder FunctionBuilder::Operator(Operator::BuiltInOperator op, NativeFunctionSignature impl)
{
  FunctionBuilder ret{ Function::OperatorFunction };
  ret.operation = op;
  ret.callback = impl;
  return ret;
}

FunctionBuilder FunctionBuilder::Operator(Operator::BuiltInOperator op, const Type & rt, const Type & a, NativeFunctionSignature impl)
{
  FunctionBuilder ret{ Function::OperatorFunction };
  ret.operation = op;
  ret.proto = Prototype{ rt, a };
  ret.callback = impl;
  return ret;
}

FunctionBuilder FunctionBuilder::Operator(Operator::BuiltInOperator op, const Type & rt, const Type & a, const Type & b, NativeFunctionSignature impl)
{
  FunctionBuilder ret{ Function::OperatorFunction };
  ret.operation = op;
  ret.proto = Prototype{ rt, a, b };
  ret.callback = impl;
  return ret;
}

FunctionBuilder FunctionBuilder::Operator(Operator::BuiltInOperator op, const Prototype & proto, NativeFunctionSignature impl)
{
  FunctionBuilder ret{ Function::OperatorFunction };
  ret.operation = op;
  ret.proto = proto;
  ret.callback = impl;
  return ret;
}


FunctionBuilder FunctionBuilder::Cast(const Type & srcType, const Type & destType, NativeFunctionSignature impl)
{
  FunctionBuilder ret{ Function::CastFunction };
  ret.proto = Prototype{ destType, Type::ref(srcType).withFlag(Type::ThisFlag) };
  ret.callback = impl;
  return ret;
}


FunctionBuilder & FunctionBuilder::setConst()
{
  this->proto.setParameter(0, Type::cref(this->proto.argv(0)));
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

  if (proto.argc() == 0 || !proto.argv(0).testFlag(Type::ThisFlag))
    return *this;

  for (int i(0); i < proto.argc() - 1; ++i)
    proto.setParameter(i, proto.argv(i+1));
  proto.popArgument();

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
  this->proto.addArgument(t);
  return *(this);
}

script::Function FunctionBuilder::create()
{
  if (this->engine == nullptr)
    throw std::runtime_error{ "FunctionBuilder::create() : null engine" };

  if (is_member_function())
  {
    Class cla = member_of();
    script::Function f = this->engine->newFunction(*this);
    if (f.isOperator())
      cla.implementation()->operators.push_back(f.toOperator());
    else if (f.isCast())
      cla.implementation()->casts.push_back(f.toCast());
    else if (f.isConstructor())
      cla.implementation()->registerConstructor(f);
    else if (f.isDestructor())
      cla.implementation()->destructor = f;
    else
      cla.implementation()->register_function(f);
    return f;
  }
  else if (!this->namespace_scope.isNull())
  {
    Namespace ns = this->namespace_scope;
    script::Function f = this->engine->newFunction(*this);
    if (f.isOperator())
      ns.implementation()->operators.push_back(f.toOperator());
    else if (f.isLiteralOperator())
      ns.implementation()->literal_operators.push_back(f.toLiteralOperator());
    else
      ns.implementation()->functions.push_back(f);
    return f;
  }

  throw std::runtime_error{ "FunctionBuilder::create() : not implemented" };
}

bool FunctionBuilder::is_member_function() const
{
  return this->special.isNull() == false || (this->proto.argc() > 0 && this->proto.argv(0).testFlag(Type::ThisFlag));
}

Class FunctionBuilder::member_of() const
{
  if (!this->special.isNull())
    return this->special;

  return this->engine->getClass(this->proto.argv(0));
}

} // namespace script
