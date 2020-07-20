// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/commandcompiler.h"

#include "script/compiler/compiler.h"
#include "script/compiler/compilererrors.h"
#include "script/compiler/compilesession.h"
#include "script/compiler/lambdacompiler.h"

#include "script/parser/parser.h"
#include "script/parser/parsererrors.h"

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
      std::string name = oper.arg1->as<ast::SimpleIdentifier>().getName();

      auto value = generateExpression(oper.arg2);
      return program::BindExpression::New(std::move(name), context_, value);
    }
  }

  return ExpressionCompiler::generateOperation(op);
}

CommandCompiler::CommandCompiler(Compiler *c)
  : Component(c),
    expr_(c)
{

}

std::shared_ptr<program::Expression> CommandCompiler::compile(const std::string & expr, Context context)
{
  auto source = SourceFile::fromString(expr);
  parser::Parser parser{ source };

  std::shared_ptr<ast::AST> ast = nullptr;

  try
  {
    ast = parser.parseExpression(source);
  }
  catch (parser::SyntaxError& ex)
  {
    DiagnosticMessage mssg = session()->messageBuilder().error(ex);
    CompilationFailure exception{ CompilerError::SyntaxError, mssg.content() };
    // @TODO: do not bind location to the source file which is not supposed to exist outside this function
    exception.location.m_source = source;
    exception.location.m_pos = source.map(ex.offset);
    throw exception;
  } 

  // @TODO: write source location if 'compile' throws CompilationFailure
  return compile(std::static_pointer_cast<ast::Expression>(ast->root), context);
}

std::shared_ptr<program::Expression> CommandCompiler::compile(const std::shared_ptr<ast::Expression> & expr, const Context & context)
{
  expr_.context_ = context;
  expr_.setScope(context.scope());
  return expr_.generateExpression(expr);
}

} // namespace compiler

} // namespace script

