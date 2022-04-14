// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_CAST_P_H
#define LIBSCRIPT_CAST_P_H

#include "script/private/programfunction.h"

namespace script
{

class CastImpl : public ProgramFunction
{
public:
  CastPrototype proto_;

public:
  CastImpl(const Prototype &p, Engine *e, FunctionFlags f);
  ~CastImpl() = default;

  SymbolKind get_kind() const override;
  Name get_name() const override;
  const Prototype & prototype() const override;

  bool is_native() const override;
};

} // namespace script


#endif // LIBSCRIPT_CAST_P_H
