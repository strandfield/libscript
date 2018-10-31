// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTIONBODY_H
#define LIBSCRIPT_FUNCTIONBODY_H

#include "script/callbacks.h"

#include <memory>

namespace script
{

namespace program
{
class Statement;
} // namespace program

struct LIBSCRIPT_API FunctionBody
{
  NativeFunctionSignature callback;
  std::shared_ptr<program::Statement> program;

  FunctionBody()
    : callback(nullptr)
  {

  }
};

} // namespace script

#endif // LIBSCRIPT_FUNCTIONBODY_H
