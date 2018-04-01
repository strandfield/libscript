// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_BUILDER_H
#define LIBSCRIPT_FUNCTION_BUILDER_H

#include "script/function.h"
#include "script/operator.h"
#include "script/cast.h"
#include "script/class.h"

namespace script
{

class LIBSCRIPT_API FunctionBuilder
{
public:
  NativeFunctionSignature callback;
  Function::Kind kind;
  std::string name;
  Prototype proto;
  int flags;
  Class special;
  Operator::BuiltInOperator operation;
  std::shared_ptr<UserData> data;

  static FunctionBuilder Function(const std::string & name, const Prototype & proto, NativeFunctionSignature impl = nullptr);
  static FunctionBuilder Constructor(const Class & cla, Prototype proto, NativeFunctionSignature impl = nullptr);
  static FunctionBuilder Constructor(const Class & cla, NativeFunctionSignature impl = nullptr);
  static FunctionBuilder Destructor(const Class & cla, NativeFunctionSignature impl = nullptr);
  static FunctionBuilder Method(const Class & cla, const std::string & name, NativeFunctionSignature impl = nullptr);

  static FunctionBuilder Operator(Operator::BuiltInOperator op);
  static FunctionBuilder Operator(Operator::BuiltInOperator op, NativeFunctionSignature impl = nullptr);
  static FunctionBuilder Operator(Operator::BuiltInOperator op, const Type & rt, const Type & a, NativeFunctionSignature impl = nullptr);
  static FunctionBuilder Operator(Operator::BuiltInOperator op, const Type & rt, const Type & a, const Type & b, NativeFunctionSignature impl = nullptr);
  static FunctionBuilder Operator(Operator::BuiltInOperator op, const Prototype & proto, NativeFunctionSignature impl = nullptr);

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

  FunctionBuilder & setReturnType(const Type & t);
  FunctionBuilder & addParam(const Type & t);

protected:
  FunctionBuilder(Function::Kind k);

};

} // namespace script

#endif // LIBSCRIPT_FUNCTION_BUILDER_H