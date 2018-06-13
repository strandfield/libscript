// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_BUILDER_H
#define LIBSCRIPT_FUNCTION_BUILDER_H

#include "script/function.h"
#include "script/operator.h"
#include "script/cast.h"
#include "script/class.h"
#include "script/namespace.h"

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
  Namespace namespace_scope;
  Class special;
  Operator::BuiltInOperator operation;
  std::shared_ptr<UserData> data;

public:
  FunctionBuilder(Function::Kind k);
  FunctionBuilder(Class cla, Function::Kind k);
  FunctionBuilder(Class cla, Operator::BuiltInOperator op);
  FunctionBuilder(Namespace ns);
  FunctionBuilder(Namespace ns, Operator::BuiltInOperator op);

  static FunctionBuilder Function(const std::string & name, const Prototype & proto, NativeFunctionSignature impl = nullptr);
  static FunctionBuilder Constructor(const Class & cla, Prototype proto, NativeFunctionSignature impl = nullptr);
  static FunctionBuilder Constructor(const Class & cla, NativeFunctionSignature impl = nullptr);
  static FunctionBuilder Destructor(const Class & cla, NativeFunctionSignature impl = nullptr);
  [[deprecated("use builder functions in Class instead")]] static FunctionBuilder Method(const Class & cla, const std::string & name, NativeFunctionSignature impl = nullptr);

  [[deprecated("use builder functions in Namespace and Class instead")]] static FunctionBuilder Operator(Operator::BuiltInOperator op, NativeFunctionSignature impl = nullptr);
  [[deprecated("use builder functions in Namespace and Class instead")]] static FunctionBuilder Operator(Operator::BuiltInOperator op, const Prototype & proto, NativeFunctionSignature impl = nullptr);

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