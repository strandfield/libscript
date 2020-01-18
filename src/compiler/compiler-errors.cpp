// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/errors.h"
#include "script/compiler/compilererrors.h"

namespace script
{

namespace errors
{

class CompilerCategory : public std::error_category
{
public:

  const char* name() const noexcept override
  {
    return "compiler-category";
  }

  std::string message(int) const override
  {
    return "compiler-error";
  }
};

const std::error_category& compiler_category() noexcept
{
  static CompilerCategory static_instance = {};
  return static_instance;
}

} // namespace errors

namespace compiler
{

CompilerErrorData::~CompilerErrorData()
{

}

} // namespace compiler

} // namespace script

