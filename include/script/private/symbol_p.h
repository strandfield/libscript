// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_SYMBOL_P_H
#define LIBSCRIPT_SYMBOL_P_H

#include "libscriptdefs.h"

#include <memory>

namespace script
{

class Script;

class SymbolImpl
{
public:
  std::weak_ptr<SymbolImpl> enclosing_symbol;

public:
  virtual ~SymbolImpl() = default;
};

} // namespace script

#endif // LIBSCRIPT_SYMBOL_P_H
