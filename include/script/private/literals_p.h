// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_LITERALS_P_H
#define LIBSCRIPT_LITERALS_P_H

#include "script/private/function_p.h"

namespace script
{

class LiteralOperatorImpl : public FunctionImpl
{
public:
  std::string suffix;
  DynamicPrototype proto_;
public:
  LiteralOperatorImpl(std::string && suffix, const Prototype & proto, Engine *engine, FunctionFlags flags);
  ~LiteralOperatorImpl() = default;

  Name get_name() const override;
  const Prototype & prototype() const override;
};

} // namespace script


#endif // LIBSCRIPT_LITERALS_P_H
