// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/functionbuilder.h"

namespace script
{

FunctionBuilder::FunctionBuilder(Function::Kind k)
  : callback(nullptr)
  , kind(k)
  , flags(0)
  , operation(Operator::Null)
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


FunctionBuilder FunctionBuilder::Operator(Operator::BuiltInOperator op)
{
  FunctionBuilder ret{ Function::OperatorFunction };
  ret.operation = op;
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


} // namespace script
