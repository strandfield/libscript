// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_VARIABLE_ACCESSOR_H
#define LIBSCRIPT_COMPILER_VARIABLE_ACCESSOR_H

#include "script/diagnosticmessage.h"

#include <memory>
#include <vector>

namespace script
{

namespace program
{
class CaptureAccess;
class Expression;
} // namespace program

namespace compiler
{

class ExpressionCompiler;
class Stack;

class VariableAccessor
{
private:
  Stack* stack_;
  std::vector<std::shared_ptr<program::CaptureAccess>> captures_;

public:
  explicit VariableAccessor(Stack* s = nullptr);
  VariableAccessor(const VariableAccessor &) = delete;
  ~VariableAccessor() = default;

  void setStack(Stack* s);
  const Stack& stack() const;

  const std::vector<std::shared_ptr<program::CaptureAccess>>& generatedCaptures() const { return captures_; }

  /// TODO: maybe pass ast::Expression instead of dpos
  std::shared_ptr<program::Expression> accessDataMember(ExpressionCompiler & ec, int offset);
  std::shared_ptr<program::Expression> accessGlobal(ExpressionCompiler & ec, int offset);
  std::shared_ptr<program::Expression> accessLocal(ExpressionCompiler & ec, int offset);
  std::shared_ptr<program::Expression> accessCapture(ExpressionCompiler & ec, int offset);
  /// TODO: add accessStaticDataMember

  static std::shared_ptr<program::Expression> generateMemberAccess(ExpressionCompiler & ec, const std::shared_ptr<program::Expression> & object, const int index);

  VariableAccessor & operator=(const VariableAccessor &) = delete;

};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_VARIABLE_ACCESSOR_H
