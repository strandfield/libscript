// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_OPERATORBUILDER_H
#define LIBSCRIPT_OPERATORBUILDER_H

#include "script/functionbuilder.h"

#include "script/operators.h"

namespace script
{

class Operator;

class LIBSCRIPT_API OperatorBuilder : public GenericFunctionBuilder<OperatorBuilder>
{
public:
  typedef BinaryOperatorPrototype prototype_t;

public:
  OperatorName operation;
  prototype_t proto;

public:
  OperatorBuilder(const Symbol & s, OperatorName op);

  OperatorBuilder & setConst();
  OperatorBuilder & setDeleted();
  OperatorBuilder & setDefaulted();

  OperatorBuilder & setReturnType(const Type & t);
  OperatorBuilder & addParam(const Type & t);

  void create();
  script::Operator get();
};

class LIBSCRIPT_API FunctionCallOperatorBuilder : public GenericFunctionBuilder<FunctionCallOperatorBuilder>
{
public:
  typedef DynamicPrototype prototype_t;

public:
  prototype_t proto_;
  std::vector<std::shared_ptr<program::Expression>> defaultargs_;

public:
  FunctionCallOperatorBuilder(const Symbol & s);

  FunctionCallOperatorBuilder & setConst();
  FunctionCallOperatorBuilder & setDeleted();

  FunctionCallOperatorBuilder & setReturnType(const Type & t);
  FunctionCallOperatorBuilder & addParam(const Type & t);

  FunctionCallOperatorBuilder & addDefaultArgument(const std::shared_ptr<program::Expression> & value);

  void create();
  script::Operator get();
};

} // namespace script

#endif // LIBSCRIPT_OPERATORBUILDER_H
