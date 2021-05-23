// Copyright (C) 2018-2021 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_CASTBUILDER_H
#define LIBSCRIPT_CASTBUILDER_H

#include "script/functionbuilder.h"

namespace script
{

class Cast;

class LIBSCRIPT_API CastBuilder : public GenericFunctionBuilder<CastBuilder>
{
public:
  typedef CastPrototype prototype_t;

public:
  prototype_t proto;

public:
  explicit CastBuilder(const Symbol& s);
  CastBuilder(const Symbol& s, const Type& dest);

  CastBuilder & setConst();
  CastBuilder & setDeleted();
  CastBuilder & setExplicit();

  CastBuilder & setReturnType(const Type & t);
  CastBuilder & addParam(const Type & t);

  CastBuilder& operator()(const Type& dest);

  void create();
  script::Cast get();
};

} // namespace script

#endif // LIBSCRIPT_CASTBUILDER_H
