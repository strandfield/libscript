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

  Derived & setData(const std::shared_ptr<UserData> & d)
  {
    data = d;
    return *(static_cast<Derived*>(this));
  }

  Derived & setAccessibility(AccessSpecifier aspec)
  {
    this->flags.set(aspec);
    return *(static_cast<Derived*>(this));
  }

  Derived & setPublic() { return setAccessibility(AccessSpecifier::Public); }
  Derived & setProtected() { return setAccessibility(AccessSpecifier::Protected); }
  Derived & setPrivate() { return setAccessibility(AccessSpecifier::Private); }

  bool isStatic() const
  {
    return flags.test(FunctionSpecifier::Static);
  }

  inline Derived & returns(const Type & t) { return static_cast<Derived*>(this)->setReturnType(t); }

  inline Derived& params() { return *static_cast<Derived*>(this); }
  inline Derived& params(const Type & arg) { return static_cast<Derived*>(this)->addParam(arg); }

  template<typename...Args>
  Derived & params(const Type & arg, const Args &... args)
  {
    static_cast<Derived*>(this)->addParam(arg);
    return params(args...);
  }

  template<typename Func>
  Derived & apply(Func && func)
  {
    func(*(static_cast<Derived*>(this)));
    return *(static_cast<Derived*>(this));
  }
};

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