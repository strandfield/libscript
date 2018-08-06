// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_BUILDER_H
#define LIBSCRIPT_FUNCTION_BUILDER_H

#include "script/function.h" /// TODO: remove this include
#include "script/symbol.h"

namespace script
{

class LIBSCRIPT_API FunctionBuilder
{
public:
  Engine *engine;
  NativeFunctionSignature callback;
  Function::Kind kind;
  std::string name;
  Prototype proto;
  int flags;
  Symbol symbol;
  OperatorName operation;
  std::shared_ptr<UserData> data;

public:
  struct LiteralOperatorTag {};

public:
  FunctionBuilder(Function::Kind k);
  FunctionBuilder(Class cla, Function::Kind k);
  FunctionBuilder(Class cla, OperatorName op);
  FunctionBuilder(Namespace ns);
  FunctionBuilder(Namespace ns, OperatorName op);
  FunctionBuilder(Namespace ns, LiteralOperatorTag, const std::string & suffix);

  static FunctionBuilder Function(const std::string & name, const Prototype & proto, NativeFunctionSignature impl = nullptr);
  static FunctionBuilder Destructor(const Class & cla, NativeFunctionSignature impl = nullptr);

  static FunctionBuilder Cast(const Type & srcType, const Type & destType, NativeFunctionSignature impl = nullptr);

  FunctionBuilder & setConst();
  FunctionBuilder & setVirtual();
  FunctionBuilder & setPureVirtual();
  FunctionBuilder & setDeleted();
  FunctionBuilder & setDefaulted();
  FunctionBuilder & setConstExpr();
  FunctionBuilder & setExplicit();
  FunctionBuilder & setPrototype(const Prototype & proto);
  FunctionBuilder & setCallback(NativeFunctionSignature impl);
  FunctionBuilder & setData(const std::shared_ptr<UserData> & data);
  FunctionBuilder & setAccessibility(AccessSpecifier aspec);
  FunctionBuilder & setPublic();
  FunctionBuilder & setProtected();
  FunctionBuilder & setPrivate();
  FunctionBuilder & setStatic();

  bool isStatic() const;

  FunctionBuilder & setReturnType(const Type & t);
  inline FunctionBuilder & returns(const Type & t) { return setReturnType(t); }

  FunctionBuilder & addParam(const Type & t);
  inline FunctionBuilder & params(const Type & arg) { return addParam(arg); }

  template<typename...Args>
  FunctionBuilder & params(const Type & arg, const Args &... args)
  {
    addParam(arg);
    return params(args...);
  }

  script::Function create();

protected:
  bool is_member_function() const;
  Class member_of() const;
};

} // namespace script

#endif // LIBSCRIPT_FUNCTION_BUILDER_H