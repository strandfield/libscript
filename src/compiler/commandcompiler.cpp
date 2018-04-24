// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/commandcompiler.h"

#include "script/compiler/compilererrors.h"
#include "script/compiler/lambdacompiler.h"

#include "script/program/expression.h"

#include "../scope_p.h"

namespace script
{

namespace compiler
{

std::shared_ptr<program::Expression> CommandExpressionCompiler::generateOperation(const std::shared_ptr<ast::Expression> & op)
{
  if (context_.isNull())
    return ExpressionCompiler::generateOperation(op);

  const ast::Operation & oper = op->as<ast::Operation>();
  if (oper.operatorToken == parser::Token::Eq)
  {
    if (oper.arg1->type() == ast::NodeType::SimpleIdentifier)
    {
      std::string name = oper.arg1->as<ast::Identifier>().getName();

      auto value = generateExpression(oper.arg2);
      return program::BindExpression::New(std::move(name), context_, value);
    }
  }

  return ExpressionCompiler::generateOperation(op);
}

CapturelessLambdaProcessor::CapturelessLambdaProcessor(Compiler *c, CompileSession *s)
  : compiler_(c)
  , session_(s)
{

}

std::shared_ptr<program::LambdaExpression> CapturelessLambdaProcessor::generate(ExpressionCompiler & ec, const std::shared_ptr<ast::LambdaExpression> & le)
{
  const auto & p = le->pos();

  if (le->captures.size() > 0)
    throw LambdaMustBeCaptureless{ diagnostic::pos_t{ p.line, p.col } };

  CompileLambdaTask task;
  task.lexpr = le;
  task.scope = script::Scope{ compiler_->engine()->rootNamespace() }; /// TODO : make this customizable !

  LambdaCompiler compiler{ compiler_, session_ };
  LambdaCompilationResult result = compiler.compile(task);

  return result.expression;
}

CommandCompiler::CommandCompiler(Compiler *c, CompileSession *s)
  : CompilerComponent(c, s)
  , lambda_(c, s)
{
  expr_.setLambdaProcessor(lambda_);
}

void CommandCompiler::setScope(const Scope & scp)
{
  expr_.setScope(scp);
}

std::shared_ptr<program::Expression> CommandCompiler::compile(const std::shared_ptr<ast::Expression> & expr, const Context & context)
{
  expr_.context_ = context;
  expr_.setScope(Scope{ std::make_shared<ContextScope>(expr_.context_, expr_.scope().impl()) });
  return expr_.generateExpression(expr);
}

std::shared_ptr<program::Expression> CommandCompiler::compile(const std::shared_ptr<ast::Expression> & expr, const Scope & scp)
{
  expr_.context_ = Context{};
  expr_.setScope(scp);
  return expr_.generateExpression(expr);
}

} // namespace compiler

} // namespace script

