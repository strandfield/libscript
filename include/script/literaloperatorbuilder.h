// Copyright (C) 2018-2021 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_LITERALOPERATORBUILDER_H
#define LIBSCRIPT_LITERALOPERATORBUILDER_H

#include "script/functionbuilder.h"

#include "script/operators.h"

namespace script
{

class LiteralOperator;

/*!
 * \class LiteralOperatorBuilder
 * \brief The LiteralOperatorBuilder class is an utility class used to build \t{LiteralOperator}s.
 *
 * See \t GenericFunctionBuilder for a description of builder classes.
 */

class LIBSCRIPT_API LiteralOperatorBuilder : public GenericFunctionBuilder<LiteralOperatorBuilder>
{
public:
  typedef UnaryOperatorPrototype prototype_t;

public:
  std::string name_;
  prototype_t proto_;

public:
  explicit LiteralOperatorBuilder(const Symbol& s);
  LiteralOperatorBuilder(const Symbol & s, std::string && suffix);
  LiteralOperatorBuilder(const Namespace& ns, std::string suffix);

  LiteralOperatorBuilder & setDeleted();

  LiteralOperatorBuilder & setReturnType(const Type & t);
  LiteralOperatorBuilder & addParam(const Type & t);

  LiteralOperatorBuilder& operator()(std::string suffix);

  void create();
  script::LiteralOperator get();
};

} // namespace script

#endif // LIBSCRIPT_LITERALOPERATORBUILDER_H
