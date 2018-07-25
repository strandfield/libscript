// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_VARIABLE_ACCESSOR_H
#define LIBSCRIPT_COMPILER_VARIABLE_ACCESSOR_H


/// TODO: review these includes
#include "script/ast/forwards.h"
#include "script/program/expression.h"
#include "script/diagnosticmessage.h"

namespace script
{

namespace compiler
{

class ExpressionCompiler;

class VariableAccessor2
{
public:
  VariableAccessor2() = default;
  VariableAccessor2(const VariableAccessor2 &) = delete;
  virtual ~VariableAccessor2() = default;

  /// TODO: maybe pass ast::Expression instead of dpos
  virtual std::shared_ptr<program::Expression> accessDataMember(ExpressionCompiler & ec, int offset, const diagnostic::pos_t dpos);
  virtual std::shared_ptr<program::Expression> accessGlobal(ExpressionCompiler & ec, int offset, const diagnostic::pos_t dpos);
  virtual std::shared_ptr<program::Expression> accessLocal(ExpressionCompiler & ec, int offset, const diagnostic::pos_t dpos);
  virtual std::shared_ptr<program::Expression> accessCapture(ExpressionCompiler & ec, int offset, const diagnostic::pos_t dpos);
  /// TODO: add accessStaticDataMember

  VariableAccessor2 & operator=(const VariableAccessor2 &) = delete;

protected:
  std::shared_ptr<program::Expression> generateMemberAccess(ExpressionCompiler & ec, const std::shared_ptr<program::Expression> & object, const int index, const diagnostic::pos_t dpos);

};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_VARIABLE_ACCESSOR_H
