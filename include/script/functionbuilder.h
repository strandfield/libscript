// Copyright (C) 2018-2021 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_BUILDER_H
#define LIBSCRIPT_FUNCTION_BUILDER_H

#include "script/function-blueprint.h"

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

class LiteralOperator;

/*!
 * \class FunctionBuilder
 * \brief The FunctionBuilder class is an utility class used to build \t{Function}s.
 */

class LIBSCRIPT_API FunctionBuilder
{
public:
  FunctionBlueprint blueprint_;

public:
  FunctionBuilder(Symbol s, SymbolKind k, std::string name);
  FunctionBuilder(Symbol s, SymbolKind k, Type t);
  FunctionBuilder(Symbol s, SymbolKind k, OperatorName n);

  explicit FunctionBuilder(Symbol s);
  explicit FunctionBuilder(FunctionBlueprint blueprint);

  static FunctionBuilder Fun(Class c, std::string name);
  static FunctionBuilder Fun(Namespace ns, std::string name);
  static FunctionBuilder Constructor(Class c);
  static FunctionBuilder Destructor(Class c);
  static FunctionBuilder Op(Class c, OperatorName op);
  static FunctionBuilder Op(Namespace ns, OperatorName op);
  static FunctionBuilder LiteralOp(Namespace ns, std::string suffix);
  static FunctionBuilder Cast(Class c);

  FunctionBuilder& setCallback(NativeFunctionSignature impl);
  FunctionBuilder& setProgram(std::shared_ptr<program::Statement> prog);

  FunctionBuilder& setData(std::shared_ptr<UserData> d);

  FunctionBuilder& setAccessibility(AccessSpecifier aspec);

  FunctionBuilder& setPublic();
  FunctionBuilder& setProtected();
  FunctionBuilder& setPrivate();

  bool isStatic() const;

  FunctionBuilder& returns(const Type& t);

  FunctionBuilder& params();
  FunctionBuilder& params(const Type& arg);

  template<typename...Args>
  FunctionBuilder& params(const Type& arg, const Args &... args);

  template<typename Func>
  FunctionBuilder& apply(Func&& func);

  static  Value throwing_body(FunctionCall*);

  FunctionBuilder& setConst();
  FunctionBuilder& setVirtual();
  FunctionBuilder& setPureVirtual();
  FunctionBuilder& setDeleted();
  FunctionBuilder& setDefaulted();
  FunctionBuilder& setExplicit();
  FunctionBuilder& setPrototype(const Prototype & proto);
  FunctionBuilder& setStatic();

  FunctionBuilder& setReturnType(const Type & t);
  FunctionBuilder& addParam(const Type & t);

  // @TODO: remove
  [[deprecated("could be removed at any time")]]
  FunctionBuilder& operator()(std::string name);

  void create();
  script::Function get();
};

/*!
 * \fn FunctionBuilder& setCallback(NativeFunctionSignature impl)
 * \brief Sets the callback of the function.
 *
 */
inline FunctionBuilder& FunctionBuilder::setCallback(NativeFunctionSignature impl)
{
  blueprint_.body_ = builders::make_body(impl);
  return *this;
}

inline FunctionBuilder& FunctionBuilder::setProgram(std::shared_ptr<program::Statement> prog)
{
  blueprint_.body_ = prog;
  return *this;
}

/*!
 * \fn FunctionBuilder& setData(std::shared_ptr<UserData> d)
 * \brief Sets the function user data.
 *
 */
inline FunctionBuilder& FunctionBuilder::setData(std::shared_ptr<UserData> d)
{
  blueprint_.data_ = d;
  return *this;
}

/*!
 * \fn FunctionBuilder& setAccessibility(AccessSpecifier aspec)
 * \brief Sets the function accessibility.
 *
 */
inline FunctionBuilder& FunctionBuilder::setAccessibility(AccessSpecifier aspec)
{
  blueprint_.flags_.set(aspec);
  return *this;
}

/*!
 * \fn FunctionBuilder& setPublic()
 * \brief Sets the function accessibility to \c public.
 *
 */
inline FunctionBuilder& FunctionBuilder::setPublic()
{ 
  return setAccessibility(AccessSpecifier::Public); 
}

/*!
 * \fn FunctionBuilder& setProtected()
 * \brief Sets the function accessibility to \c protected.
 *
 */
inline FunctionBuilder& FunctionBuilder::setProtected()
{ 
  return setAccessibility(AccessSpecifier::Protected); 
}

/*!
 * \fn FunctionBuilder& setPrivate()
 * \brief Sets the function accessibility to \c private.
 *
 */
inline FunctionBuilder& FunctionBuilder::setPrivate()
{ 
  return setAccessibility(AccessSpecifier::Private); 
}

inline bool FunctionBuilder::isStatic() const
{
  return blueprint_.flags_.test(FunctionSpecifier::Static);
}

/*!
 * \fn FunctionBuilder& returns(const Type & t)
 * \brief Sets the return type of the function.
 *
 * If not specified, the return type is \c void.
 */
inline FunctionBuilder& FunctionBuilder::returns(const Type& t)
{ 
  return setReturnType(t); 
}

/*!
 * \fn FunctionBuilder& params(const Type & arg, const Args &... args)
 * \brief Adds parameters to the function.
 *
 * Note that this function does not set the parameter list but rather adds
 * parameters.
 */
inline FunctionBuilder& FunctionBuilder::params() 
{ 
  return *this; 
}

inline FunctionBuilder& FunctionBuilder::params(const Type& arg) 
{ 
  return addParam(arg); 
}

template<typename...Args>
inline FunctionBuilder& FunctionBuilder::params(const Type& arg, const Args&... args)
{
  addParam(arg);
  return params(args...);
}

/*!
 * \fn FunctionBuilder& apply(Func && func)
 * \tparam{Func}{a callable object}
 * \brief Applies a function to the builder.
 *
 * Note that the callable object must either be targeted at a specific builder class
 * or use template (e.g. generic lambdas).
 */
template<typename Func>
inline FunctionBuilder& FunctionBuilder::apply(Func&& func)
{
  func(*this);
  return *this;
}

/*!
 * \endclass
 */

} // namespace script

#endif // LIBSCRIPT_FUNCTION_BUILDER_H