// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_LITERALS_P_H
#define LIBSCRIPT_LITERALS_P_H

#include "function_p.h"

namespace script
{

class LiteralOperatorImpl : public FunctionImpl
{
public:
  std::string suffix;
public:
  LiteralOperatorImpl(std::string && suffix, const Prototype & proto, Engine *engine, uint8 flags = 0);
  ~LiteralOperatorImpl() = default;

  std::string name() const override
  {
    throw std::runtime_error{ "Not implemented : LiteralOperator::name()" };
  }
};

} // namespace script


#endif // LIBSCRIPT_LITERALS_P_H
