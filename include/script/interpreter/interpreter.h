// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_INTERPRETER_H
#define LIBSCRIPT_INTERPRETER_H

#include "libscriptdefs.h"

#include "script/program/statements.h"
#include "script/program/expression.h"

#include "script/interpreter/executioncontext.h"

namespace script
{

namespace interpreter
{

class LIBSCRIPT_API Interpreter : public program::StatementVisitor, public program::ExpressionVisitor
{
public:
  Interpreter(std::shared_ptr<ExecutionContext> ec, Engine *e);
  ~Interpreter();

  Value invoke(const Function & f, const Value *obj, const Value *begin, const Value *end);

  Value eval(const std::shared_ptr<program::Expression> & expr);

  void exec(program::Statement & s);
  void exec(const std::shared_ptr<program::Statement> & s);

protected:
  bool evalCondition(const std::shared_ptr<program::Expression> & expr);
  void evalForSideEffects(const std::shared_ptr<program::Expression> & expr);
  Value inner_eval(const std::shared_ptr<program::Expression> & expr);
  Value manage(const Value & val);
  void invoke(const Function & f);

private:
  // StatementVisitor
  void visit(const program::BreakStatement &) override;
  void visit(const program::CompoundStatement &) override;
  void visit(const program::ContinueStatement &) override;
  void visit(const program::InitObjectStatement &) override;
  void visit(const program::ExpressionStatement &) override;
  void visit(const program::ForLoop &) override;
  void visit(const program::IfStatement &) override;
  void visit(const program::ConstructionStatement &) override;
  void visit(const program::PushDataMember &) override;
  void visit(const program::PushGlobal &) override;
  void visit(const program::PushValue &) override;
  void visit(const program::PopDataMember &) override;
  void visit(const program::PopValue &) override;
  void visit(const program::ReturnStatement &) override;
  void visit(const program::WhileLoop &) override;

  // ExpressionVisitor
  Value visit(const program::ArrayExpression &) override;
  Value visit(const program::BindExpression &) override;
  Value visit(const program::CaptureAccess &) override;
  Value visit(const program::CommaExpression &) override;
  Value visit(const program::ConditionalExpression &) override;
  Value visit(const program::ConstructorCall &) override;
  Value visit(const program::Copy &) override;
  Value visit(const program::FetchGlobal &) override;
  Value visit(const program::FunctionCall &) override;
  Value visit(const program::FunctionVariableCall &) override;
  Value visit(const program::FundamentalConversion &) override;
  Value visit(const program::InitializerList &) override;
  Value visit(const program::LambdaExpression &) override;
  Value visit(const program::Literal &) override;
  Value visit(const program::LogicalAnd &) override;
  Value visit(const program::LogicalOr &) override;
  Value visit(const program::MemberAccess &) override;
  Value visit(const program::StackValue &) override;
  Value visit(const program::VariableAccess &) override;
  Value visit(const program::VirtualCall &) override;

private:
  std::shared_ptr<ExecutionContext> mExecutionContext;
  Engine *mEngine;
};

} // namespace interpreter

} // namespace script

#endif // LIBSCRIPT_INTERPRETER_H
