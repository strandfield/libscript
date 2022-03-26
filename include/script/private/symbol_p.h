// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_SYMBOL_P_H
#define LIBSCRIPT_SYMBOL_P_H

#include "libscriptdefs.h"

#include <memory>

namespace script
{

class Function;
class Name;
class Script;
class Symbol;

class SymbolImpl
{
public:
  std::weak_ptr<SymbolImpl> enclosing_symbol;

public:
  SymbolImpl() = default;
  explicit SymbolImpl(const std::shared_ptr<SymbolImpl> & parent) : enclosing_symbol(parent) { }
  virtual ~SymbolImpl() = default;

  virtual Name get_name() const = 0;
};

void add_function_to_symbol(const Function& func, Symbol& parent);

} // namespace script

#endif // LIBSCRIPT_SYMBOL_P_H
