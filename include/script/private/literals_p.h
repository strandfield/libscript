// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_LITERALS_P_H
#define LIBSCRIPT_LITERALS_P_H

#include "script/private/programfunction.h"

namespace script
{

class LiteralOperatorImpl : public ProgramFunction
{
public:
  std::string suffix;
  DynamicPrototype proto_;

public:
  LiteralOperatorImpl(std::string suffix, const Prototype & proto, Engine *engine, FunctionFlags flags);
  ~LiteralOperatorImpl() = default;

  SymbolKind get_kind() const override;
  const std::string& literal_operator_suffix() const override;
  Name get_name() const override;
  const Prototype & prototype() const override;

  bool is_native() const override;
};

} // namespace script


#endif // LIBSCRIPT_LITERALS_P_H
