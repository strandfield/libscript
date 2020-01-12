// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_VARIABLE_PROCESSOR_H
#define LIBSCRIPT_COMPILER_VARIABLE_PROCESSOR_H

#include "script/scope.h"

#include "script/compiler/expressioncompiler.h"

#include "script/ast/forwards.h"
#include "script/program/expression.h"

namespace script
{

namespace compiler
{

class ExpressionCompiler;

class VariableProcessor : public program::ExpressionVisitor
{
private:
  struct Variable
  {
    Variable() { }
    Variable(const Value & var, const std::shared_ptr<ast::VariableDecl> & decl, const Scope & scp) :
      variable(var), declaration(decl), scope(scp) { }

    Value variable;
    std::shared_ptr<ast::VariableDecl> declaration;
    Scope scope;
  };

  Engine *engine_;
  std::vector<Variable> uninitialized_variables_;
  ExpressionCompiler expr_;
  TypeResolver type_;

public:
  VariableProcessor(Compiler *c);
  ~VariableProcessor() = default;

  inline Engine* engine() const { return engine_; }

  inline ExpressionCompiler & expressionCompiler() { return expr_; }
  inline TypeResolver & typeResolver() { return type_; }

  void process(const std::shared_ptr<ast::VariableDecl> & decl, const Scope & scp);
  void initializeVariables();
  inline bool empty() const { return uninitialized_variables_.empty(); }

protected:
  void process_namespace_variable(const std::shared_ptr<ast::VariableDecl> & decl, const Scope & scp);
  void process_data_member(const std::shared_ptr<ast::VariableDecl> & decl, const Scope & scp);
  
  void initialize(Variable v);
  void default_initialization(Variable & v);
  void copy_initialization(Variable & var, const std::shared_ptr<program::Expression> & value);
  void constructor_initialization(Variable & var, const std::shared_ptr<program::ConstructorCall> & call);

  Value eval(const std::shared_ptr<program::Expression> & e);
  Value manage(const Value & v);

protected:
  // program::ExpressionVisitor
  Value visit(const program::ArrayExpression & ae);
  Value visit(const program::BindExpression &);
  Value visit(const program::CaptureAccess &);
  Value visit(const program::CommaExpression &);
  Value visit(const program::ConditionalExpression & ce);
  Value visit(const program::ConstructorCall & cc);
  Value visit(const program::Copy &);
  Value visit(const program::FetchGlobal &);
  Value visit(const program::FunctionCall & fc);
  Value visit(const program::FunctionVariableCall & fvc);
  Value visit(const program::FundamentalConversion &);
  Value visit(const program::InitializerList &);
  Value visit(const program::LambdaExpression &);
  Value visit(const program::Literal &);
  Value visit(const program::LogicalAnd &);
  Value visit(const program::LogicalOr &);
  Value visit(const program::MemberAccess & ma);
  Value visit(const program::StackValue &);
  Value visit(const program::VariableAccess &);
  Value visit(const program::VirtualCall &);
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_VARIABLE_PROCESSOR_H
