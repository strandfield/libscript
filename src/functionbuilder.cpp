// Copyright (C) 2018-2021 Vincent Chambrin
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

#include "script/compiler/constructorcompiler.h"
#include "script/compiler/destructorcompiler.h"
#include "script/program/statements.h"

#include "script/castbuilder.h"
#include "script/constructorbuilder.h"
#include "script/destructorbuilder.h"
#include "script/literaloperatorbuilder.h"
#include "script/operatorbuilder.h"

namespace script
{

namespace builders
{

std::shared_ptr<program::Statement> make_body(NativeFunctionSignature impl)
{
  if (impl)
  {
    auto r = std::make_shared<program::CompoundStatement>();
    r->statements.push_back(std::make_shared<program::CppReturnStatement>(impl));
    return r;
  }
  else
  {
    return nullptr;
  }
}

} // namespace builders


template<typename FT, typename Builder>
static void generic_fill(const std::shared_ptr<FT>& impl, const Builder& opts)
{
  impl->program_ = opts.body;
  impl->data = opts.data;
  impl->enclosing_symbol = opts.symbol.impl();
}

static void add_to_parent(const Function & func, const Symbol & parent)
{
  /// The following is done in generic_fill
  //func.impl()->enclosing_symbol = parent.impl();

  if (parent.isClass())
  {
    Class cla = parent.toClass();
    cla.addFunction(func);
  }
  else if (parent.isNamespace())
  {
    Namespace ns = parent.toNamespace();
    ns.addFunction(func);
  }
}

inline static void set_default_args(Function & fun, std::vector<DefaultArgument> && dargs)
{
  fun.impl()->set_default_arguments(std::move(dargs));
}


/*!
 * \class FunctionBuilder
 */

FunctionBuilder::FunctionBuilder(Class cla, std::string name)
  : GenericFunctionBuilder<FunctionBuilder>(Symbol{cla})
  , name_(std::move(name))
{
  this->proto_.setReturnType(Type::Void);
  this->proto_.push(Type::ref(cla.id()).withFlag(Type::ThisFlag));
}

FunctionBuilder::FunctionBuilder(Namespace ns, std::string name)
  : GenericFunctionBuilder<FunctionBuilder>(Symbol{ ns })
  , name_(std::move(name))
{
  this->proto_.setReturnType(Type::Void);
}

FunctionBuilder::FunctionBuilder(Symbol s, std::string name)
  : GenericFunctionBuilder<FunctionBuilder>(s)
  , name_(std::move(name))
{
  this->proto_.setReturnType(Type::Void);

  if(s.isClass())
    this->proto_.push(Type::ref(s.toClass().id()).withFlag(Type::ThisFlag));
}

FunctionBuilder::FunctionBuilder(Symbol s)
  : GenericFunctionBuilder<FunctionBuilder>(s)
{

}

Value FunctionBuilder::throwing_body(FunctionCall*)
{
  throw RuntimeError{ "Called undefined function" };
}

FunctionBuilder & FunctionBuilder::setConst()
{
  this->proto_.setParameter(0, Type::cref(this->proto_.at(0)));
  return *(this);
}

FunctionBuilder & FunctionBuilder::setVirtual()
{
  this->flags.set(FunctionSpecifier::Virtual);
  return *(this);
}

FunctionBuilder & FunctionBuilder::setPureVirtual()
{
  this->flags.set(FunctionSpecifier::Virtual);
  this->flags.set(FunctionSpecifier::Pure);
  return *(this);
}

FunctionBuilder & FunctionBuilder::setDeleted()
{
  this->flags.set(FunctionSpecifier::Delete);
  return *(this);
}

FunctionBuilder & FunctionBuilder::setPrototype(const Prototype & proto)
{
  this->proto_ = proto;
  return *(this);
}

FunctionBuilder & FunctionBuilder::setStatic()
{
  this->flags.set(FunctionSpecifier::Static);

  if (proto_.count() == 0 || !proto_.at(0).testFlag(Type::ThisFlag))
    return *this;

  for (int i(0); i < proto_.count() - 1; ++i)
    proto_.setParameter(i, proto_.at(i+1));
  proto_.pop();

  return *this;
}

FunctionBuilder & FunctionBuilder::setReturnType(const Type & t)
{
  this->proto_.setReturnType(t);
  return *(this);
}

FunctionBuilder & FunctionBuilder::addParam(const Type & t)
{
  this->proto_.push(t);
  return *(this);
}

FunctionBuilder & FunctionBuilder::addDefaultArgument(const std::shared_ptr<program::Expression> & value)
{
  this->defaultargs_.push_back(value);
  return *this;
}

FunctionBuilder& FunctionBuilder::operator()(std::string name)
{
  this->flags = FunctionFlags();
  this->proto_.clear();
  this->defaultargs_.clear();

  if(symbol.isClass())
      this->proto_.push(Type::ref(symbol.toClass().id()).withFlag(Type::ThisFlag));

  this->name_ = std::move(name);

  return *this;
}

/*!
 * \fn void create()
 * \brief Creates the function object.
 *
 * Creates the function object, without returning it.
 * This simply calls \m get.
 * This method allows you to create the object without having its complete definition.
 */

void FunctionBuilder::create()
{
  get();
}

/*!
 * \fn Function get()
 * \brief Creates and return the function object.
 */

script::Function FunctionBuilder::get()
{
  auto impl = std::make_shared<RegularFunctionImpl>(std::move(name_), std::move(proto_), engine, flags);
  generic_fill(impl, *this);
  Function ret{ impl };
  set_default_args(ret, std::move(defaultargs_));
  add_to_parent(ret, symbol);
  return ret;
}

/*!
 * \endclass
 */

/*!
 * \class OperatorBuilder
 */

OperatorBuilder::OperatorBuilder(const Symbol& s)
  : GenericFunctionBuilder<OperatorBuilder>(s)
  , operation(OperatorName::InvalidOperator)
  , proto_{ Type::Void, Type::Null, Type::Null }
{

}

OperatorBuilder::OperatorBuilder(const Symbol & s, OperatorName op)
  : GenericFunctionBuilder<OperatorBuilder>(s)
  , operation(op)
  , proto_{ Type::Void, Type::Null, Type::Null }
{
  if (symbol.isClass())
    this->proto_.setParameter(0, Type::ref(symbol.toClass().id()).withFlag(Type::ThisFlag));
}

OperatorBuilder & OperatorBuilder::setConst()
{
  this->proto_.setParameter(0, Type::cref(this->proto_.at(0)));
  return *(this);
}

OperatorBuilder & OperatorBuilder::setDeleted()
{
  this->flags.set(FunctionSpecifier::Delete);
  return *(this);
}

OperatorBuilder & OperatorBuilder::setDefaulted()
{
  this->flags.set(FunctionSpecifier::Default);
  return *(this);
}

OperatorBuilder & OperatorBuilder::setReturnType(const Type & t)
{
  this->proto_.setReturnType(t);
  return *(this);
}

OperatorBuilder & OperatorBuilder::addParam(const Type & t)
{
  if (this->proto_.at(0).isNull())
    this->proto_.setParameter(0, t);
  else
    this->proto_.setParameter(1, t);

  return *(this);
}

OperatorBuilder& OperatorBuilder::operator()(OperatorName op)
{
  this->flags = FunctionFlags();
  this->proto_.setReturnType(Type::Void);
  this->proto_.setParameter(0, Type::Null);
  this->proto_.setParameter(1, Type::Null);

  if (symbol.isClass())
    this->proto_.setParameter(0, Type::ref(symbol.toClass().id()).withFlag(Type::ThisFlag));

  this->operation = op;

  return *this;
}

void OperatorBuilder::create()
{
  get();
}

script::Operator OperatorBuilder::get()
{
  std::shared_ptr<OperatorImpl> impl;

  if (Operator::isBinary(operation))
    impl = std::make_shared<BinaryOperatorImpl>(operation, proto_, engine, flags);
  else
    impl = std::make_shared<UnaryOperatorImpl>(operation, proto_, engine, flags);

  generic_fill(impl, *this);
  add_to_parent(Function{ impl }, symbol);
  return Operator{ impl };
}

/*!
 * \endclass
 */

 /*!
  * \class FunctionCallOperatorBuilder
  */

FunctionCallOperatorBuilder::FunctionCallOperatorBuilder(const Symbol & s)
  : GenericFunctionBuilder<FunctionCallOperatorBuilder>(s)
  , proto_(Type::Void, {})
{
  if (!s.isClass())
    throw std::runtime_error{ "Function call operator cannot be non-member" };

  proto_.push(Type::ref(s.toClass().id()).withFlag(Type::ThisFlag));
}

FunctionCallOperatorBuilder & FunctionCallOperatorBuilder::setConst()
{
  this->proto_.setParameter(0, Type::cref(this->proto_.at(0)));
  return *(this);
}

FunctionCallOperatorBuilder & FunctionCallOperatorBuilder::setDeleted()
{
  this->flags.set(FunctionSpecifier::Delete);
  return *(this);
}


FunctionCallOperatorBuilder & FunctionCallOperatorBuilder::setReturnType(const Type & t)
{
  this->proto_.setReturnType(t);
  return *this;
}

FunctionCallOperatorBuilder & FunctionCallOperatorBuilder::addParam(const Type & t)
{
  this->proto_.push(t);
  return *this;
}

FunctionCallOperatorBuilder & FunctionCallOperatorBuilder::addDefaultArgument(const std::shared_ptr<program::Expression> & value)
{
  defaultargs_.push_back(value);
  return *this;
}

FunctionCallOperatorBuilder& FunctionCallOperatorBuilder::operator()()
{
  this->flags = FunctionFlags();
  this->proto_.clear();
  this->defaultargs_.clear();

  if (symbol.isClass())
    this->proto_.push(Type::ref(symbol.toClass().id()).withFlag(Type::ThisFlag));

  return *this;
}

void FunctionCallOperatorBuilder::create()
{
  get();
}

script::Operator FunctionCallOperatorBuilder::get()
{
  auto impl = std::make_shared<FunctionCallOperatorImpl>(OperatorName::FunctionCallOperator, std::move(proto_), engine, flags);
  generic_fill(impl, *this);
  Operator ret{ impl };
  set_default_args(ret, std::move(defaultargs_));
  add_to_parent(ret, symbol);
  return ret;
}

/*!
 * \endclass
 */

 /*!
  * \class CastBuilder
  */

CastBuilder::CastBuilder(const Class& cla)
  : GenericFunctionBuilder<CastBuilder>(Symbol(cla))
  , proto(Type::Null, Type::Null)
{
  proto.setParameter(0, Type::ref(cla.id()).withFlag(Type::ThisFlag));
}

CastBuilder::CastBuilder(const Symbol& s)
  : GenericFunctionBuilder<CastBuilder>(s)
  , proto(Type::Null, Type::Null)
{
  if (!s.isClass())
  {
    throw std::runtime_error{ "Conversion functions can only be members" };
  }

  proto.setParameter(0, Type::ref(s.toClass().id()).withFlag(Type::ThisFlag));
}

CastBuilder::CastBuilder(const Symbol & s, const Type & dest)
  : GenericFunctionBuilder<CastBuilder>(s)
  , proto(dest, Type::Null)
{
  if (!s.isClass())
  {
    throw std::runtime_error{ "Conversion functions can only be members" };
  }

  proto.setParameter(0, Type::ref(s.toClass().id()).withFlag(Type::ThisFlag));
}

CastBuilder & CastBuilder::setConst()
{
  this->proto.setParameter(0, Type::cref(this->proto.at(0)));
  return *(this);
}

CastBuilder & CastBuilder::setDeleted()
{
  this->flags.set(FunctionSpecifier::Delete);
  return *(this);
}

CastBuilder & CastBuilder::setExplicit()
{
  this->flags.set(FunctionSpecifier::Explicit);
  return *(this);
}

CastBuilder & CastBuilder::setReturnType(const Type & t)
{
  this->proto.setReturnType(t);
  return *this;
}

CastBuilder & CastBuilder::addParam(const Type & t)
{
  throw std::runtime_error{ "Cannot add parameter to conversion function" };
}

CastBuilder& CastBuilder::operator()(const Type& dest)
{
  this->flags = FunctionFlags();
  this->proto.setReturnType(dest);
  this->proto.setParameter(0, Type::ref(symbol.toClass().id()).withFlag(Type::ThisFlag));

  return *this;
}

void CastBuilder::create()
{
  get();
}

script::Cast CastBuilder::get()
{
  auto impl = std::make_shared<CastImpl>(proto, engine, flags);
  generic_fill(impl, *this);
  add_to_parent(Function{ impl }, symbol);
  return Cast{ impl };
}

/*!
 * \endclass
 */

 /*!
  * \class ConstructorBuilder
  */

ConstructorBuilder::ConstructorBuilder(const Class& cla)
  : GenericFunctionBuilder<ConstructorBuilder>(Symbol(cla))
  , proto_(Type::Void, {})
{
  proto_.push(Type::ref(cla.id()).withFlag(Type::ThisFlag));
}

ConstructorBuilder::ConstructorBuilder(const Symbol & s)
  : GenericFunctionBuilder<ConstructorBuilder>(s)
  , proto_(Type::Void, {})
{
  if (!s.isClass())
    throw std::runtime_error{ "Constructors can only be members" };

  proto_.push(Type::ref(s.toClass().id()).withFlag(Type::ThisFlag));
}

ConstructorBuilder & ConstructorBuilder::setDefaulted()
{
  this->flags.set(FunctionSpecifier::Default);
  return *(this);
}

ConstructorBuilder & ConstructorBuilder::setDeleted()
{
  this->flags.set(FunctionSpecifier::Delete);
  return *(this);
}

ConstructorBuilder & ConstructorBuilder::setExplicit()
{
  this->flags.set(FunctionSpecifier::Explicit);
  return *(this);
}

ConstructorBuilder & ConstructorBuilder::setReturnType(const Type & t)
{
  if (t != Type::Void)
    throw std::runtime_error{ "Constructors must return void" };

  return *this;
}

ConstructorBuilder & ConstructorBuilder::addParam(const Type & t)
{
  proto_.push(t);
  return *this;
}

ConstructorBuilder & ConstructorBuilder::addDefaultArgument(const std::shared_ptr<program::Expression> & value)
{
  defaultargs_.push_back(value);
  return *this;
}

ConstructorBuilder & ConstructorBuilder::compile()
{
  if (!flags.test(FunctionSpecifier::Default))
    throw std::runtime_error{ "ConstructorBuilder : only defaulted function can be compiled" };


  if (proto_.count() > 2)
    throw std::runtime_error{ "ConstructorBuilder : function cannot be defaulted" };

  if (proto_.count() == 1)
  {
    this->body = compiler::ConstructorCompiler::generateDefaultConstructor(symbol.toClass());
    this->flags.set(ImplementationMethod::InterpretedFunction);
  }
  else
  {
    if (proto_.at(1) == Type::cref(symbol.toClass().id()))
    {
      this->body = compiler::ConstructorCompiler::generateCopyConstructor(symbol.toClass());
      this->flags.set(ImplementationMethod::InterpretedFunction);
    }
    else if (proto_.at(1) == Type::rref(symbol.toClass().id()))
    {
      this->body = compiler::ConstructorCompiler::generateMoveConstructor(symbol.toClass());
      this->flags.set(ImplementationMethod::InterpretedFunction);
    }
    else
    {
      throw std::runtime_error{ "ConstructorBuilder : function cannot be defaulted" };
    }
  }

  return *this;
}

ConstructorBuilder& ConstructorBuilder::operator()()
{
  this->flags = FunctionFlags();
  this->proto_.clear();
  this->defaultargs_.clear();

  proto_.push(Type::ref(symbol.toClass().id()).withFlag(Type::ThisFlag));

  return *this;
}

void ConstructorBuilder::create()
{
  get();
}

script::Function ConstructorBuilder::get()
{
  auto impl = std::make_shared<ConstructorImpl>(proto_, engine, flags);
  generic_fill(impl, *this);
  Function ret{ impl };
  set_default_args(ret, std::move(defaultargs_));
  add_to_parent(ret, symbol);
  return ret;
}

/*!
 * \endclass
 */

 /*!
  * \class DestructorBuilder
  */

DestructorBuilder::DestructorBuilder(const Class& cla)
  : GenericFunctionBuilder<DestructorBuilder>(Symbol(cla))
  , proto_(Type::Void, Type::Null)
{
  proto_.setParameter(0, Type::ref(cla.id()).withFlag(Type::ThisFlag));
}

DestructorBuilder::DestructorBuilder(const Symbol & s)
  : GenericFunctionBuilder<DestructorBuilder>(s)
  , proto_(Type::Void, Type::Null)
{
  if (!s.isClass())
    throw std::runtime_error{ "Destructor can only be members" };

  proto_.setParameter(0, Type::ref(s.toClass().id()).withFlag(Type::ThisFlag));
}

DestructorBuilder & DestructorBuilder::setDefaulted()
{
  this->flags.set(FunctionSpecifier::Default);
  return *(this);
}

DestructorBuilder & DestructorBuilder::setVirtual()
{
  this->flags.set(FunctionSpecifier::Virtual);
  return *(this);
}

DestructorBuilder & DestructorBuilder::setReturnType(const Type & t)
{
  if (t != Type::Void)
    throw std::runtime_error{ "Destructors must have return type void" };

  return *this;
}

DestructorBuilder & DestructorBuilder::addParam(const Type & t)
{
  throw std::runtime_error{ "Cannot add parameter to destructor" };
}

DestructorBuilder & DestructorBuilder::compile()
{
  if (!flags.test(FunctionSpecifier::Default))
    throw std::runtime_error{ "DestructorBuilder : only defaulted function can be compiled" };

  this->body = compiler::DestructorCompiler::generateDestructor(symbol.toClass());
  this->flags.set(ImplementationMethod::InterpretedFunction);

  return *this;
}

void DestructorBuilder::create()
{
  get();
}

script::Function DestructorBuilder::get()
{
  auto impl = std::make_shared<DestructorImpl>(proto_, engine, flags);
  generic_fill(impl, *this);
  Function ret{ impl };
  add_to_parent(ret, symbol);
  return ret;
}

/*!
 * \endclass
 */

 /*!
  * \class LiteralOperatorBuilder
  */

LiteralOperatorBuilder::LiteralOperatorBuilder(const Symbol& s)
  : GenericFunctionBuilder<LiteralOperatorBuilder>(s)
  , proto_(Type::Null, Type::Null)
{

}


LiteralOperatorBuilder::LiteralOperatorBuilder(const Symbol & s, std::string && suffix)
  : GenericFunctionBuilder<LiteralOperatorBuilder>(s)
  , name_(std::move(suffix))
  , proto_(Type::Null, Type::Null)
{

}

LiteralOperatorBuilder::LiteralOperatorBuilder(const Namespace& ns, std::string suffix)
  : GenericFunctionBuilder<LiteralOperatorBuilder>(Symbol(ns))
  , name_(std::move(suffix))
  , proto_(Type::Null, Type::Null)
{

}

LiteralOperatorBuilder & LiteralOperatorBuilder::setDeleted()
{
  this->flags.set(FunctionSpecifier::Delete);
  return *(this);
}

LiteralOperatorBuilder & LiteralOperatorBuilder::setReturnType(const Type & t)
{
  this->proto_.setReturnType(t);
  return *this;
}

LiteralOperatorBuilder & LiteralOperatorBuilder::addParam(const Type & t)
{
  this->proto_.setParameter(0, t);
  return *this;
}

LiteralOperatorBuilder& LiteralOperatorBuilder::operator()(std::string suffix)
{
  this->flags = FunctionFlags();
  this->proto_.setReturnType(Type::Null);
  this->proto_.setParameter(0, Type::Null);

  name_ = std::move(suffix);

  return *(this);
}

void LiteralOperatorBuilder::create()
{
  get();
}

script::LiteralOperator LiteralOperatorBuilder::get()
{
  auto impl = std::make_shared<LiteralOperatorImpl>(std::move(name_), proto_, engine, flags);
  generic_fill(impl, *this);
  add_to_parent(Function{ impl }, symbol);
  return LiteralOperator{ impl };
}

} // namespace script
