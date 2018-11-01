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

/*!
 * \class GenericFunctionBuilder
 * \brief Template class providing common functionalities of builder classes.
 * \tparam{Derived}{derived type}
 * 
 * The GenericFunctionBuilder is a template class that provides defaults for all 
 * the functionalities of a function builder.
 * Function builders are used to create \t{Function}s or other derived types.
 *
 * This class is never used directly; you only use the derived types, depending 
 * on the kind of function you want to create.
 * \begin{list}
 *   \li \t FunctionBuilder
 *   \li \t ConstructorBuilder
 *   \li \t DestructorBuilder
 *   \li \t OperatorBuilder
 *   \li \t LiteralOperatorBuilder
 *   \li \t FunctionCallOperatorBuilder
 *   \li \t CastBuilder
 * \end{list}
 *
 * The curiously recurring template is used to inject the behavior of the derived class
 * into its base and to allow calls to the member functions to be chained (sometimes referred 
 * to as a "fluent" API).
 *
 * \begin[cpp]{code}
 * Class c = ...;
 * c.Method("foo", callback)
 *   .setStatic()
 *   .returns(Type::Int)
 *   .params(Type::Int, Type::Boolean)
 *   .create();
 * \end{code}
 *
 * Relevant function builders can be obtained directly from a \t Class or \t Namespace, 
 * as in the above example, so you will rarely ever need to type the name of the builders.
 *
 * The builder classes do not have any common base class (different instances of a 
 * class template do not share a common base; i.e., \c{std::vector<int>} and 
 * \c{std::vector<float>} are not part of a common class hierarchy), so the correct way 
 * to use the similarities between all builder classes is through templates.
 *
 * \begin[cpp]{code}
 * template<typename Builder>
 * void foo(Builder & b)
 * {
 *   b.setStatic().returns(Type::Void);
 * }
 * \end{code}
 *
 * Each builder class is optimized for its use case and will exhibit a behavior that 
 * is consistent with the object it is supposed to create. 
 * For example, calling \m setStatic on a \t DestructorBuilder will throw an exception.
 *
 * Functionalities provided by all builder classes are presented in this class, even 
 * though they may only be implemented in the derived classes.
 * The GenericFunctionBuilder class only provides defaults, some of which are hidden 
 * by derived classes (through C++ function hiding).
 */


/*!
 * \fn Derived & setConst()
 * \brief Makes the function \c const.
 *
 * This only works for member functions, member operators and conversion functions 
 * and will throw if called for another target.
 */

/*!
 * \fn Derived & setVirtual()
 * \brief Makes the function \c virtual.
 *
 * This only works for member functions and destructors
 * and will throw if called for another target.
 */

/*!
 * \fn Derived & setPureVirtual()
 * \brief Makes the function a \c virtual pure function.
 *
 * This only works for member functions and will throw if called for another target.
 */

/*!
 * \fn Derived & setDeleted()
 * \brief Marks the function with the \c delete specifier.
 *
 */

/*!
 * \fn Derived & setDefaulted()
 * \brief Makes the function \c{= default}.
 *
 * This can only be used for default, copy & move constructors, for destructors 
 * and assignment operator.
 */

/*!
 * \fn Derived & setConstExpr()
 * \brief Makes the function a \c constexpr function.
 *
 * Not implemented yet, will throw.
 */

/*!
 * \fn Derived & setExplicit()
 * \brief Makes the function \c explicit.
 *
 * This can only be used for single-parameter constructors and conversion functions.
 */

/*!
 * \fn Derived & setCallback(NativeFunctionSignature impl)
 * \brief Sets the callback of the function.
 *
 */

/*!
 * \fn Derived & setData(const std::shared_ptr<UserData> & d)
 * \brief Sets the function user data.
 *
 */

/*!
 * \fn Derived & setAccessibility(AccessSpecifier aspec)
 * \brief Sets the function accessibility.
 *
 */

/*!
 * \fn Derived & setPublic()
 * \brief Sets the function accessibility to \c public.
 *
 */

/*!
 * \fn Derived & setProtected()
 * \brief Sets the function accessibility to \c protected.
 *
 */

/*!
 * \fn Derived & setPrivate()
 * \brief Sets the function accessibility to \c private.
 *
 */

/*!
 * \fn Derived & setStatic()
 * \brief Makes the function \c static.
 *
 * Only makes sense for member functions.
 */

/*!
 * \fn Derived & returns(const Type & t)
 * \brief Sets the return type of the function.
 *
 * If not specified, the return type is \c void.
 */

/*!
 * \fn Derived & params(const Type & arg, const Args &... args)
 * \brief Adds parameters to the function.
 *
 * Note that this function does not set the parameter list but rather adds
 * parameters.
 */

/*!
 * \fn Derived & addDefaultArgument(const std::shared_ptr<program::Expression> & value)
 * \brief Adds a default argument to the function.
 *
 * Default arguments must be provided in reverse order as the parameters; i.e., 
 * the last parameter gets a default first, than the penultimate parameter and so on.
 *
 * Currently this function does not perform any type checking, but will probably in the future.
 */

/*!
 * \fn Derived & compile()
 * \brief Generates the body of a defaulted function.
 *
 * After calling \m setDefaulted, you may call this function to make the 
 * library generate the function body.
 * Currently this only works for destructors, default, copy & move constructors.
 */

/*!
 * \fn Derived & apply(Func && func)
 * \tparam{Func}{a callable object}
 * \brief Applies a function to the builder.
 *
 * Note that the callable object must either be targeted at a specific builder class 
 * or use template (e.g. generic lambdas).
 */

/*!
 * \fn T get()
 * \brief Creates and return the function object.
 *
 * The return type corresponds to the object you want to create.
 */

 /*!
 * \fn void create()
 * \brief Creates the function object.
 *
 * Creates the function object, without returning it.
 * This simply calls \m get.
 * This method allows you to create the object without having its complete definition.
 */

template<typename Builder>
static void generic_fill(const std::shared_ptr<FunctionImpl> & impl, const Builder & opts)
{
  impl->implementation = opts.body;
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

    if (func.isOperator())
      cla.impl()->operators.push_back(func.toOperator());
    else if (func.isCast())
      cla.impl()->casts.push_back(func.toCast());
    else if (func.isConstructor())
      cla.impl()->registerConstructor(func);
    else if (func.isDestructor())
      cla.impl()->destructor = func;
    else
      cla.impl()->register_function(func);
  }
  else if (parent.isNamespace())
  {
    Namespace ns = parent.toNamespace();

    if (func.isOperator())
      ns.impl()->operators.push_back(func.toOperator());
    else if (func.isLiteralOperator())
      ns.impl()->literal_operators.push_back(func.toLiteralOperator());
    else
      ns.impl()->functions.push_back(func);
  }
}

inline static void set_default_args(Function & fun, std::vector<DefaultArgument> && dargs)
{
  fun.impl()->set_default_arguments(std::move(dargs));
}


/*!
 * \endclass
 * \class FunctionBuilder
 * \brief The FunctionBuilder class is an utility class used to build \t{Function}s.
 *
 * See \t GenericFunctionBuilder for a description of builder classes.
 */

FunctionBuilder::FunctionBuilder(Class cla, std::string && name)
  : GenericFunctionBuilder<FunctionBuilder>(Symbol{cla})
  , name_(std::move(name))
{
  this->proto_.setReturnType(Type::Void);
  this->proto_.push(Type::ref(cla.id()).withFlag(Type::ThisFlag));
}

FunctionBuilder::FunctionBuilder(Namespace ns, std::string && name)
  : GenericFunctionBuilder<FunctionBuilder>(Symbol{ ns })
  , name_(std::move(name))
{
  this->proto_.setReturnType(Type::Void);
}

FunctionBuilder::FunctionBuilder(Symbol s, std::string && name)
  : GenericFunctionBuilder<FunctionBuilder>(s)
  , name_(std::move(name))
{
  this->proto_.setReturnType(Type::Void);

  if(s.isClass())
    this->proto_.push(Type::ref(s.toClass().id()).withFlag(Type::ThisFlag));
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

void FunctionBuilder::create()
{
  get();
}

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
 * \class OperatorBuilder
 * \brief The OperatorBuilder class is an utility class used to build \t{Operator}s.
 *
 * See \t GenericFunctionBuilder for a description of builder classes.
 */

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

inline OperatorBuilder & OperatorBuilder::setReturnType(const Type & t)
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
 * \class FunctionCallOperatorBuilder
 * \brief The FunctionCallOperatorBuilder class is an utility class used to build a function call \t{Operator}.
 *
 * See \t GenericFunctionBuilder for a description of builder classes.
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
 * \class CastBuilder
 * \brief The CastBuilder class is an utility class used to build \t{Cast}s.
 *
 * See \t GenericFunctionBuilder for a description of builder classes.
 */

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
 * \class ConstructorBuilder
 * \brief The ConstructorBuilder class is an utility class used to build constructors.
 *
 * See \t GenericFunctionBuilder for a description of builder classes.
 */

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
    this->body.program = compiler::ConstructorCompiler::generateDefaultConstructor(symbol.toClass());
    this->flags.set(ImplementationMethod::InterpretedFunction);
  }
  else
  {
    if (proto_.at(1) == Type::cref(symbol.toClass().id()))
    {
      this->body.program = compiler::ConstructorCompiler::generateCopyConstructor(symbol.toClass());
      this->flags.set(ImplementationMethod::InterpretedFunction);
    }
    else if (proto_.at(1) == Type::rref(symbol.toClass().id()))
    {
      this->body.program = compiler::ConstructorCompiler::generateMoveConstructor(symbol.toClass());
      this->flags.set(ImplementationMethod::InterpretedFunction);
    }
    else
    {
      throw std::runtime_error{ "ConstructorBuilder : function cannot be defaulted" };
    }
  }

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
 * \class DestructorBuilder
 * \brief The DestructorBuilder class is an utility class used to build destructors.
 *
 * See \t GenericFunctionBuilder for a description of builder classes.
 */

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

  this->body.program = compiler::DestructorCompiler::generateDestructor(symbol.toClass());
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
 * \class LiteralOperatorBuilder
 * \brief The LiteralOperatorBuilder class is an utility class used to build \t{LiteralOperator}s.
 *
 * See \t GenericFunctionBuilder for a description of builder classes.
 */

LiteralOperatorBuilder::LiteralOperatorBuilder(const Symbol & s, std::string && suffix)
  : GenericFunctionBuilder<LiteralOperatorBuilder>(s)
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
