// Copyright (C) 2018-2021 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_OPERATORBUILDER_H
#define LIBSCRIPT_OPERATORBUILDER_H

#include "script/functionbuilder.h"

#include "script/operators.h"

namespace script
{

class Operator;

/*!
 * \class OperatorBuilder
 * \brief The OperatorBuilder class is an utility class used to build \t{Operator}s.
 *
 * See \t GenericFunctionBuilder for a description of builder classes.
 */

//class LIBSCRIPT_API OperatorBuilder : public GenericFunctionBuilder<OperatorBuilder>
//{
//public:
//  typedef BinaryOperatorPrototype prototype_t;
//
//public:
//  OperatorName operation;
//  prototype_t proto_;
//
//public:
//  explicit OperatorBuilder(const Symbol& s);
//  OperatorBuilder(const Symbol & s, OperatorName op);
//
//  OperatorBuilder & setConst();
//  OperatorBuilder & setDeleted();
//  OperatorBuilder & setDefaulted();
//
//  OperatorBuilder & setReturnType(const Type & t);
//  OperatorBuilder & addParam(const Type & t);
//
//  OperatorBuilder& operator()(OperatorName op);
//
//  void create();
//  script::Operator get();
//};

/*!
 * \endclass
 */

 /*!
  * \class FunctionCallOperatorBuilder
  * \brief The FunctionCallOperatorBuilder class is an utility class used to build a function call \t{Operator}.
  *
  * See \t GenericFunctionBuilder for a description of builder classes.
  */

//class LIBSCRIPT_API FunctionCallOperatorBuilder : public GenericFunctionBuilder<FunctionCallOperatorBuilder>
//{
//public:
//  typedef DynamicPrototype prototype_t;
//
//public:
//  prototype_t proto_;
//  std::vector<std::shared_ptr<program::Expression>> defaultargs_;
//
//public:
//  FunctionCallOperatorBuilder(const Symbol & s);
//
//  FunctionCallOperatorBuilder & setConst();
//  FunctionCallOperatorBuilder & setDeleted();
//
//  FunctionCallOperatorBuilder & setReturnType(const Type & t);
//  FunctionCallOperatorBuilder & addParam(const Type & t);
//
//  FunctionCallOperatorBuilder & addDefaultArgument(const std::shared_ptr<program::Expression> & value);
//
//  FunctionCallOperatorBuilder& operator()();
//
//  void create();
//  script::Operator get();
//};

} // namespace script

#endif // LIBSCRIPT_OPERATORBUILDER_H
