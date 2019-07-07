// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMMAND_COMPILER_H
#define LIBSCRIPT_COMMAND_COMPILER_H

#include "script/compiler/expressioncompiler.h"

#include "script/context.h"

namespace script
{

namespace compiler
{

class CommandExpressionCompiler : public ExpressionCompiler
{
public:
  using ExpressionCompiler::ExpressionCompiler;

  Context context_;

private:
  std::shared_ptr<program::Expression> generateOperation(const std::shared_ptr<ast::Expression> & op) override;
};

class CommandCompiler
{
public:
  explicit CommandCompiler(Engine *e);
  ~CommandCompiler() = default;

  inline Engine* engine() const { return mEngine; }

  std::shared_ptr<program::Expression> compile(const std::string & expr, Context context);
  std::shared_ptr<program::Expression> compile(const std::shared_ptr<ast::Expression> & expr, const Context & context);

protected:

private:
  Engine *mEngine;
  CommandExpressionCompiler expr_;
};


} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMMAND_COMPILER_H
