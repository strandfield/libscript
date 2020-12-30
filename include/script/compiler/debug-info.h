// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_DEBUGINFO_H
#define LIBSCRIPT_COMPILER_DEBUGINFO_H

#include "script/types.h"

#include "script/utils/stringview.h"

#include <memory>

namespace script
{

namespace compiler
{

class DebugInfoBlock
{
public:
  script::Type vartype;
  std::string varname;
  std::shared_ptr<DebugInfoBlock> prev;

  DebugInfoBlock(script::Type t, std::string n, std::shared_ptr<DebugInfoBlock> p)
    : vartype(t),
      varname(std::move(n)),
      prev(p)
  {

  }

  static std::shared_ptr<DebugInfoBlock> fetch(std::shared_ptr<DebugInfoBlock> block, size_t delta)
  {
    while (delta-- > 0)
    {
      block = block->prev;
    }

    return block;
  }
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_STACK_H
