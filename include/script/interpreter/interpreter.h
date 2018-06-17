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

  Value call(const Function & f, const Value *obj, const Value *begin, const Value *end);
  Value call(const Function & f, const std::vector<Value> & args);

  Value invoke(const Function & f, const Value *obj, const Value *begin, const Value *end);

  template<typename Iter>
  Value invoke(const Function & f, Iter begin, Iter end)
  {
    const int sp = mExecutionContext->stack.size;

    if (f.isConstructor())
      mExecutionContext->stack.push(createObject(f.returnType()));
    else if (f.isDestructor());
    else
      mExecutionContext->stack.push(Value{});

    for (auto it = begin; it != end; ++it)
      mExecutionContext->stack.push(*it);

    mExecutionContext->push(f, sp);

    invoke(f);

    return mExecutionContext->pop();
  }

  template<typename Iter>
  void placement(const Function & ctor, Value object, Iter begin, Iter end)
  {
    assert(ctor.isConstructor());

    const int sp = mExecutionContext->stack.size;
    mExecutionContext->stack.push(object);
    for (auto it = begin; it != end; ++it)
      mExecutionContext->stack.push(*it);

    mExecutionContext->push(ctor, sp);

    invoke(ctor);

    mExecutionContext->pop();
  }

  Value eval(program::Expression & expr);
  Value eval(const std::shared_ptr<program::Expression> & expr);

  void exec(program::Statement & s);
  void exec(const std::shared_ptr<program::Statement> & s);

protected:
  void invoke(const Function & f);
  Value createObject(const Type & t);

private:
  // StatementVisitor
  void visit(const program::BreakStatement &) override;
  void visit(const program::CompoundStatement &) override;
  void visit(const program::ContinueStatement &) override;
  void visit(const program::InitObjectStatement &) override;
  void visit(const program::ExpressionStatement &) override;
  void visit(const program::ForLoop &) override;
  void visit(const program::IfStatement &) override;
  void visit(const program::PlacementStatement &) override;
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
