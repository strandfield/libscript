// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_LITERALOPERATORBUILDER_H
#define LIBSCRIPT_LITERALOPERATORBUILDER_H

#include "script/functionbuilder.h"

#include "script/operators.h"

namespace script
{

class LiteralOperator;

class LIBSCRIPT_API LiteralOperatorBuilder : public GenericFunctionBuilder<LiteralOperatorBuilder>
{
public:
  typedef UnaryOperatorPrototype prototype_t;

public:
  std::string name_;
  prototype_t proto_;

public:
  LiteralOperatorBuilder(const Symbol & s, std::string && suffix);

  LiteralOperatorBuilder & setDeleted();

  LiteralOperatorBuilder & setReturnType(const Type & t);
  LiteralOperatorBuilder & addParam(const Type & t);

  void create();
  script::LiteralOperator get();
};

} // namespace script

#endif // LIBSCRIPT_LITERALOPERATORBUILDER_H
