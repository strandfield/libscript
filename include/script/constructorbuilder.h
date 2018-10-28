// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_CONSTRUCTORBUILDER_H
#define LIBSCRIPT_CONSTRUCTORBUILDER_H

#include "script/functionbuilder.h"

namespace script
{

class Function;

class LIBSCRIPT_API ConstructorBuilder : public GenericFunctionBuilder<ConstructorBuilder>
{
public:
  typedef DynamicPrototype prototype_t;

public:
  prototype_t proto_;
  std::vector<std::shared_ptr<program::Expression>> defaultargs_;

public:
  ConstructorBuilder(const Symbol & s);

  ConstructorBuilder & setDefaulted();
  ConstructorBuilder & setDeleted();
  ConstructorBuilder & setExplicit();

  ConstructorBuilder & setReturnType(const Type & t);
  ConstructorBuilder & addParam(const Type & t);

  ConstructorBuilder & addDefaultArgument(const std::shared_ptr<program::Expression> & value);

  void create();
  script::Function get();
};

} // namespace script

#endif // LIBSCRIPT_CONSTRUCTORBUILDER_H
