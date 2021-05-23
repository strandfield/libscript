// Copyright (C) 2018-2021 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_BUILDER_H
#define LIBSCRIPT_FUNCTION_BUILDER_H

#include "script/accessspecifier.h"
#include "script/callbacks.h"
#include "script/functionflags.h"
#include "script/prototypes.h"
#include "script/symbol.h"
#include "script/userdata.h"

#include <memory>
#include <vector>

namespace script
{

namespace program
{
class Expression;
class Statement;
} // namespace program

namespace builders
{

std::shared_ptr<program::Statement> make_body(NativeFunctionSignature impl);

} // namespace builders

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
 * FunctionBuilder(c, "foo").setCallback(callback)
 *   .setStatic()
 *   .returns(Type::Int)
 *   .params(Type::Int, Type::Boolean)
 *   .create();
 * \end{code}
 *
 */

template<typename Derived>
class GenericFunctionBuilder
{
public:
  Engine *engine;
  std::shared_ptr<program::Statement> body;
  FunctionFlags flags;
  Symbol symbol;
  std::shared_ptr<UserData> data;

public:
  GenericFunctionBuilder(const Symbol & s)
    : symbol(s) 
  {
    engine = s.engine(); 
  }
  
  /*!
   * \fn Derived & setCallback(NativeFunctionSignature impl)
   * \brief Sets the callback of the function.
   *
   */
  Derived & setCallback(NativeFunctionSignature impl)
  {
    body = builders::make_body(impl);
    flags.set(ImplementationMethod::NativeFunction);
    return *(static_cast<Derived*>(this));
  }

  Derived & setProgram(const std::shared_ptr<program::Statement> & prog)
  {
    body = prog;
    flags.set(ImplementationMethod::InterpretedFunction);
    return *(static_cast<Derived*>(this));
  }

  /*!
   * \fn Derived & setData(const std::shared_ptr<UserData> & d)
   * \brief Sets the function user data.
   *
   */
  Derived & setData(const std::shared_ptr<UserData> & d)
  {
    data = d;
    return *(static_cast<Derived*>(this));
  }

  /*!
   * \fn Derived & setAccessibility(AccessSpecifier aspec)
   * \brief Sets the function accessibility.
   *
   */
  Derived & setAccessibility(AccessSpecifier aspec)
  {
    this->flags.set(aspec);
    return *(static_cast<Derived*>(this));
  }

  /*!
   * \fn Derived & setPublic()
   * \brief Sets the function accessibility to \c public.
   *
   */
  Derived & setPublic() { return setAccessibility(AccessSpecifier::Public); }

  /*!
   * \fn Derived & setProtected()
   * \brief Sets the function accessibility to \c protected.
   *
   */
  Derived & setProtected() { return setAccessibility(AccessSpecifier::Protected); }

  /*!
   * \fn Derived & setPrivate()
   * \brief Sets the function accessibility to \c private.
   *
   */
  Derived & setPrivate() { return setAccessibility(AccessSpecifier::Private); }

  bool isStatic() const
  {
    return flags.test(FunctionSpecifier::Static);
  }

  /*!
   * \fn Derived & returns(const Type & t)
   * \brief Sets the return type of the function.
   *
   * If not specified, the return type is \c void.
   */
  Derived & returns(const Type & t) { return static_cast<Derived*>(this)->setReturnType(t); }

  /*!
   * \fn Derived & params(const Type & arg, const Args &... args)
   * \brief Adds parameters to the function.
   *
   * Note that this function does not set the parameter list but rather adds
   * parameters.
   */
  inline Derived& params() { return *static_cast<Derived*>(this); }
  inline Derived& params(const Type & arg) { return static_cast<Derived*>(this)->addParam(arg); }

  template<typename...Args>
  Derived & params(const Type & arg, const Args &... args)
  {
    static_cast<Derived*>(this)->addParam(arg);
    return params(args...);
  }

  /*!
   * \fn Derived & apply(Func && func)
   * \tparam{Func}{a callable object}
   * \brief Applies a function to the builder.
   *
   * Note that the callable object must either be targeted at a specific builder class
   * or use template (e.g. generic lambdas).
   */
  template<typename Func>
  Derived & apply(Func && func)
  {
    func(*(static_cast<Derived*>(this)));
    return *(static_cast<Derived*>(this));
  }
};

/*!
 * \endclass 
 */

/*!
 * \class FunctionBuilder
 * \brief The FunctionBuilder class is an utility class used to build \t{Function}s.
 *
 * See \t GenericFunctionBuilder for a description of builder classes.
 */
class LIBSCRIPT_API FunctionBuilder : public GenericFunctionBuilder<FunctionBuilder>
{
public:
  std::string name_;
  DynamicPrototype proto_;
  std::vector<std::shared_ptr<program::Expression>> defaultargs_;

public:
  FunctionBuilder(Class cla, std::string name);
  FunctionBuilder(Namespace ns, std::string name);
  FunctionBuilder(Symbol s, std::string name);

  explicit FunctionBuilder(Symbol s);

  static  Value throwing_body(FunctionCall*);

  FunctionBuilder & setConst();
  FunctionBuilder & setVirtual();
  FunctionBuilder & setPureVirtual();
  FunctionBuilder & setDeleted();
  FunctionBuilder & setPrototype(const Prototype & proto);
  FunctionBuilder & setStatic();

  FunctionBuilder & setReturnType(const Type & t);
  FunctionBuilder & addParam(const Type & t);

  FunctionBuilder & addDefaultArgument(const std::shared_ptr<program::Expression> & value);

  FunctionBuilder& operator()(std::string name);

  void create();
  script::Function get();
};

} // namespace script

#endif // LIBSCRIPT_FUNCTION_BUILDER_H