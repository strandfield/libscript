// Copyright (C) 2018-2021 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/functionbuilder.h"

#include "script/attributes.h"
#include "script/class.h"
#include "script/private/class_p.h"
#include "script/engine.h"
#include "script/functioncreator.h"
#include "script/namespace.h"
#include "script/private/namespace_p.h"

#include "script/program/statements.h"

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

/*!
 * \class FunctionBuilder
 */

FunctionBuilder::FunctionBuilder(Symbol s, SymbolKind k, std::string name)
  : blueprint_(s)
{
  blueprint_.name_ = Name(k, name);

  blueprint_.prototype_.setReturnType(Type::Void);

  if (s.isClass())
    blueprint_.prototype_.push(Type::ref(s.toClass().id()).withFlag(Type::ThisFlag));
}

FunctionBuilder::FunctionBuilder(Symbol s, SymbolKind k, Type t)
  : blueprint_(s)
{
  blueprint_.name_ = Name(k, t);

  if (k == SymbolKind::Cast)
  {
    blueprint_.prototype_.setReturnType(t);
  }
  else if (k == SymbolKind::Constructor || k == SymbolKind::Destructor)
  {
    blueprint_.prototype_.setReturnType(Type::Void);
  }

  // @TODO: assert(s.isClass()) ?
  if (s.isClass())
    blueprint_.prototype_.push(Type::ref(s.toClass().id()).withFlag(Type::ThisFlag));
}

FunctionBuilder::FunctionBuilder(Symbol s, SymbolKind k, OperatorName n)
  : blueprint_(s)
{
  blueprint_.name_ = Name(k, n);

  blueprint_.prototype_.setReturnType(Type::Void);

  if(s.isClass())
    blueprint_.prototype_.push(Type::ref(s.toClass().id()).withFlag(Type::ThisFlag));
}

FunctionBuilder::FunctionBuilder(Symbol s)
  : blueprint_(s)
{

}

FunctionBuilder FunctionBuilder::Fun(Class c, std::string name)
{
  return FunctionBuilder(Symbol(c), SymbolKind::Function, std::move(name));
}

FunctionBuilder FunctionBuilder::Fun(Namespace ns, std::string name)
{
  return FunctionBuilder(Symbol(ns), SymbolKind::Function, std::move(name));
}

FunctionBuilder FunctionBuilder::Constructor(Class c)
{
  return FunctionBuilder(Symbol(c), SymbolKind::Constructor, Type(c.id()));
}

FunctionBuilder FunctionBuilder::Destructor(Class c)
{
  return FunctionBuilder(Symbol(c), SymbolKind::Destructor, Type(c.id()));
}

FunctionBuilder FunctionBuilder::Op(Class c, OperatorName op)
{
  return FunctionBuilder(Symbol(c), SymbolKind::Operator, op);
}

FunctionBuilder FunctionBuilder::Op(Namespace ns, OperatorName op)
{
  return FunctionBuilder(Symbol(ns), SymbolKind::Operator, op);
}

FunctionBuilder FunctionBuilder::LiteralOp(Namespace ns, std::string suffix)
{
  return FunctionBuilder(Symbol(ns), SymbolKind::LiteralOperator, std::move(suffix));
}

FunctionBuilder FunctionBuilder::Cast(Class c)
{
  return FunctionBuilder(Symbol(c), SymbolKind::Cast, Type::Null);
}

Value FunctionBuilder::throwing_body(FunctionCall*)
{
  throw RuntimeError{ "Called undefined function" };
}

FunctionBuilder & FunctionBuilder::setConst()
{
  blueprint_.prototype_.setParameter(0, Type::cref(blueprint_.prototype_.at(0)));
  return *(this);
}

FunctionBuilder & FunctionBuilder::setVirtual()
{
  blueprint_.flags_.set(FunctionSpecifier::Virtual);
  return *(this);
}

FunctionBuilder & FunctionBuilder::setPureVirtual()
{
  blueprint_.flags_.set(FunctionSpecifier::Virtual);
  blueprint_.flags_.set(FunctionSpecifier::Pure);
  return *(this);
}

FunctionBuilder & FunctionBuilder::setDeleted()
{
  blueprint_.flags_.set(FunctionSpecifier::Delete);
  return *(this);
}

FunctionBuilder& FunctionBuilder::setDefaulted()
{
  blueprint_.flags_.set(FunctionSpecifier::Default);
  return *(this);
}

FunctionBuilder& FunctionBuilder::setExplicit()
{
  blueprint_.flags_.set(FunctionSpecifier::Explicit);
  return *(this);
}

FunctionBuilder & FunctionBuilder::setPrototype(const Prototype & proto)
{
  blueprint_.prototype_ = proto;
  return *(this);
}

FunctionBuilder & FunctionBuilder::setStatic()
{
  blueprint_.setStatic();
  return *this;
}

FunctionBuilder & FunctionBuilder::setReturnType(const Type & t)
{
  blueprint_.prototype_.setReturnType(t);
  return *(this);
}

FunctionBuilder & FunctionBuilder::addParam(const Type & t)
{
  blueprint_.prototype_.push(t);
  return *(this);
}

FunctionBuilder & FunctionBuilder::addDefaultArgument(const std::shared_ptr<program::Expression> & value)
{
  blueprint_.defaultargs_.push_back(value);
  return *this;
}

FunctionBuilder& FunctionBuilder::operator()(std::string name)
{
  blueprint_.flags_ = FunctionFlags();
  blueprint_.prototype_.clear();
  blueprint_.defaultargs_.clear();

  if(blueprint_.parent_.isClass())
    blueprint_.prototype_.push(Type::ref(blueprint_.parent_.toClass().id()).withFlag(Type::ThisFlag));

  blueprint_.name_ = Name(SymbolKind::Function, name);

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
  FunctionCreator creator;
  return creator.create(blueprint_, nullptr, std::vector<Attribute>());
}

/*!
 * \endclass
 */

} // namespace script
