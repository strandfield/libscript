// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_BUILDER_H
#define LIBSCRIPT_FUNCTION_BUILDER_H

#include "script/accessspecifier.h"
#include "script/function.h" /// TODO: remove this include
#include "script/prototypes.h"
#include "script/symbol.h"

namespace script
{

template<typename Derived>
class GenericFunctionBuilder
{
public:
  Engine *engine;
  NativeFunctionSignature callback;
  int flags;
  Symbol symbol;
  std::shared_ptr<UserData> data;

public:
  GenericFunctionBuilder(const Symbol & s)
    : symbol(s) 
    , callback(nullptr)
    , flags(0)
  {
    engine = s.engine(); 
  }

  Derived & setConst()
  {
    throw std::runtime_error{ "Builder does not support 'const' specifier" };
  }

  Derived & setVirtual()
  {
    throw std::runtime_error{ "Builder does not support 'virtual' specifier" };
  }

  Derived & setPureVirtual()
  {
    throw std::runtime_error{ "Builder does not support 'virtual' specifier" };
  }

  Derived & setDeleted()
  {
    throw std::runtime_error{ "Builder does not support 'delete' specifier" };
  }

  Derived & setDefaulted()
  {
    throw std::runtime_error{ "Builder does not support 'default' specifier" };
  }

  Derived & setConstExpr()
  {
    throw std::runtime_error{ "Builder does not support 'constexpr' specifier" };
  }

  Derived & setExplicit()
  {
    throw std::runtime_error{ "Builder does not support 'explicit' specifier" };
  }
  
  Derived & setCallback(NativeFunctionSignature impl)
  {
    callback = impl;
    return *(static_cast<Derived*>(this));
  }

  Derived & setData(const std::shared_ptr<UserData> & d)
  {
    data = d;
    return *(static_cast<Derived*>(this));
  }

  Derived & setAccessibility(AccessSpecifier aspec)
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
    return *(static_cast<Derived*>(this));
  }

  Derived & setPublic() { return setAccessibility(AccessSpecifier::Public); }
  Derived & setProtected() { return setAccessibility(AccessSpecifier::Protected); }
  Derived & setPrivate() { return setAccessibility(AccessSpecifier::Private); }

  Derived & setStatic()
  {
    throw std::runtime_error{ "Builder does not support 'static' specifier" };
  }

  bool isStatic() const
  {
    return (flags >> 2) & Function::Static;
  }

  inline Derived & returns(const Type & t) { return static_cast<Derived*>(this)->setReturnType(t); }

  inline Derived & params(const Type & arg) { return static_cast<Derived*>(this)->addParam(arg); }

  template<typename...Args>
  Derived & params(const Type & arg, const Args &... args)
  {
    static_cast<Derived*>(this)->addParam(arg);
    return params(args...);
  }

  inline Derived & addDefaultArgument(const std::shared_ptr<program::Expression> & value)
  {
    throw std::runtime_error{ "Builder does not support default arguments" };
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
  FunctionBuilder(Class cla, std::string && name);
  FunctionBuilder(Namespace ns, std::string && name);
  FunctionBuilder(Symbol s, std::string && name);

  FunctionBuilder & setConst();
  FunctionBuilder & setVirtual();
  FunctionBuilder & setPureVirtual();
  FunctionBuilder & setDeleted();
  FunctionBuilder & setPrototype(const Prototype & proto);
  FunctionBuilder & setStatic();

  FunctionBuilder & setReturnType(const Type & t);
  FunctionBuilder & addParam(const Type & t);

  FunctionBuilder & addDefaultArgument(const std::shared_ptr<program::Expression> & value);

  void create();
  script::Function get();
};

} // namespace script

#endif // LIBSCRIPT_FUNCTION_BUILDER_H