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


template<typename FT>
static void generic_fill(const std::shared_ptr<FT>& impl, const FunctionBuilder& opts)
{
  impl->program_ = opts.blueprint_.body();
  impl->data = opts.blueprint_.data();
  impl->enclosing_symbol = opts.blueprint_.parent().impl();
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
  blueprint_.flags_.set(FunctionSpecifier::Static);

  if (blueprint_.prototype_.count() == 0 || !blueprint_.prototype_.at(0).testFlag(Type::ThisFlag))
    return *this;

  for (int i(0); i < blueprint_.prototype_.count() - 1; ++i)
    blueprint_.prototype_.setParameter(i, blueprint_.prototype_.at(i+1));
  blueprint_.prototype_.pop();

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
  if (blueprint_.name_.kind() == SymbolKind::Function)
  {
    auto impl = std::make_shared<RegularFunctionImpl>(blueprint_.name_.string(), std::move(blueprint_.prototype_), blueprint_.engine(), blueprint_.flags_);
    generic_fill(impl, *this);
    Function ret{ impl };
    set_default_args(ret, std::move(blueprint_.defaultargs_));
    add_to_parent(ret, blueprint_.parent_);
    return ret;
  }
  else if (blueprint_.name_.kind() == SymbolKind::Operator)
  {
    OperatorName operation = blueprint_.name_.operatorName();

    if (operation == OperatorName::FunctionCallOperator)
    {
      auto impl = std::make_shared<FunctionCallOperatorImpl>(OperatorName::FunctionCallOperator, std::move(blueprint_.prototype_), blueprint_.engine(), blueprint_.flags_);
      generic_fill(impl, *this);
      Operator ret{ impl };
      set_default_args(ret, std::move(blueprint_.defaultargs_));
      add_to_parent(ret, blueprint_.parent_);
      return ret;
    }
    else
    {
      std::shared_ptr<OperatorImpl> impl;

      if (Operator::isBinary(operation))
        impl = std::make_shared<BinaryOperatorImpl>(operation, blueprint_.prototype_, blueprint_.engine(), blueprint_.flags_);
      else
        impl = std::make_shared<UnaryOperatorImpl>(operation, blueprint_.prototype_, blueprint_.engine(), blueprint_.flags_);

      generic_fill(impl, *this);
      add_to_parent(Function(impl), blueprint_.parent_);
      return Operator{ impl };
    }
  }
  else if (blueprint_.name_.kind() == SymbolKind::Cast)
  {
    auto impl = std::make_shared<CastImpl>(blueprint_.prototype_, blueprint_.engine(), blueprint_.flags_);
    generic_fill(impl, *this);
    add_to_parent(Function(impl), blueprint_.parent_);
    return script::Cast(impl);
  }
  else if (blueprint_.name_.kind() == SymbolKind::Constructor)
  {
    auto impl = std::make_shared<ConstructorImpl>(blueprint_.prototype_, blueprint_.engine(), blueprint_.flags_);
    generic_fill(impl, *this);
    Function ret{ impl };
    set_default_args(ret, std::move(blueprint_.defaultargs_));
    add_to_parent(ret, blueprint_.parent_);
    return ret;
  }
  else if (blueprint_.name_.kind() == SymbolKind::Destructor)
  {
    auto impl = std::make_shared<DestructorImpl>(blueprint_.prototype_, blueprint_.engine(), blueprint_.flags_);
    generic_fill(impl, *this);
    Function ret{ impl };
    add_to_parent(ret, blueprint_.parent_);
    return ret;
  }
  else if (blueprint_.name_.kind() == SymbolKind::LiteralOperator)
  {
    auto impl = std::make_shared<LiteralOperatorImpl>(blueprint_.name_.string(), blueprint_.prototype_, blueprint_.engine(), blueprint_.flags_);
    generic_fill(impl, *this);
    add_to_parent(Function(impl), blueprint_.parent_);
    return LiteralOperator(impl);
  }
  else
  {
    assert(false);
    return {};
  }
}

/*!
 * \endclass
 */

} // namespace script
