// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_SYMBOL_P_H
#define LIBSCRIPT_SYMBOL_P_H

#include "libscriptdefs.h"

namespace script
{

class Script;

class SymbolImpl
{
public:
  virtual ~SymbolImpl() = default;

  static Script getScript(const std::shared_ptr<SymbolImpl> & symbol);
};

} // namespace script

#endif // LIBSCRIPT_SYMBOL_P_H
