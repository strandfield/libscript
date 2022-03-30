// Copyright (C) 2018-2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_SYMBOL_P_H
#define LIBSCRIPT_SYMBOL_P_H

#include "libscriptdefs.h"

#include <memory>

namespace script
{

class Engine;
class Function;
class Name;
class Script;
class Symbol;

class SymbolImpl
{
public:
  Engine* engine = nullptr;
  std::weak_ptr<SymbolImpl> enclosing_symbol;

public:
  explicit SymbolImpl(Engine* e, std::shared_ptr<SymbolImpl> parent = nullptr);
  virtual ~SymbolImpl() = default;

  virtual Name get_name() const = 0;
};

void add_function_to_symbol(const Function& func, Symbol& parent);

inline SymbolImpl::SymbolImpl(Engine* e, std::shared_ptr<SymbolImpl> parent) 
  : engine(e),
    enclosing_symbol(parent) 
{ 

}

} // namespace script

#endif // LIBSCRIPT_SYMBOL_P_H
