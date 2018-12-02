// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/commandcompiler.h"

#include "script/compiler/compiler.h"
#include "script/compiler/compilererrors.h"
#include "script/compiler/compilesession.h"
#include "script/compiler/lambdacompiler.h"

#include "script/parser/parser.h"

#include "script/program/expression.h"

#include "script/private/scope_p.h"

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

CapturelessLambdaProcessor::CapturelessLambdaProcessor(Engine *e)
  : engine_(e)
{

}

std::shared_ptr<program::LambdaExpression> CapturelessLambdaProcessor::generate(ExpressionCompiler & ec, const std::shared_ptr<ast::LambdaExpression> & le)
{
  const auto & p = le->pos();

  if (le->captures.size() > 0)
    throw LambdaMustBeCaptureless{ diagnostic::pos_t{ p.line, p.col } };

  CompileLambdaTask task;
  task.lexpr = le;
  task.scope = script::Scope{ engine_->rootNamespace() }; /// TODO : make this customizable !

  /// TODO: set tnp, module loader and so on
  LambdaCompiler compiler{ engine_ };
  LambdaCompilationResult result = compiler.compile(task);

  return result.expression;
}

CommandCompiler::CommandCompiler(Engine *e)
  : mEngine(e)
  , lambda_(e)
{
  expr_.setLambdaProcessor(lambda_);
}

std::shared_ptr<program::Expression> CommandCompiler::compile(const std::string & expr, Context context)
{
  auto source = SourceFile::fromString(expr);
  parser::Parser parser{ source };
  auto ast = parser.parseExpression(source);

  /// TODO: throw
  if (ast->hasErrors())
    return nullptr;

  expr_.context_ = context;
  expr_.setScope(context.scope());
  return expr_.generateExpression(ast->expression());
}

std::shared_ptr<program::Expression> CommandCompiler::compile(const std::shared_ptr<ast::Expression> & expr, const Context & context)
{
  expr_.context_ = context;
  expr_.setScope(context.scope());
  return expr_.generateExpression(expr);
}

} // namespace compiler

} // namespace script

