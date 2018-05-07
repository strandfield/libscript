// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILE_EXPRESSION_H
#define LIBSCRIPT_COMPILE_EXPRESSION_H

#include "script/compiler/compiler.h"

#include "script/compiler/nameresolver.h"
#include "script/compiler/typeresolver.h"

#include "script/context.h"
#include "script/conversions.h"
#include "script/functiontemplateprocessor.h"
#include "script/scope.h"

#include "script/ast/forwards.h"

namespace script
{

class NameLookup;
class Template;
class TemplateArgument;

namespace program
{
class Expression;
class LambdaExpression;
} // namespace program

namespace compiler
{

class ExpressionCompiler;

class LambdaProcessor
{
public:
  LambdaProcessor() = default;
  LambdaProcessor(const LambdaProcessor &) = delete;
  virtual ~LambdaProcessor() = default;

  virtual std::shared_ptr<program::LambdaExpression> generate(ExpressionCompiler & ec, const std::shared_ptr<ast::LambdaExpression> & le);

  LambdaProcessor & operator=(const LambdaProcessor &) = delete;
};

class VariableAccessor
{
public:
  VariableAccessor() = default;
  VariableAccessor(const VariableAccessor &) = delete;
  virtual ~VariableAccessor() = default;

  virtual std::shared_ptr<program::Expression> data_member(ExpressionCompiler & ec, int offset, const diagnostic::pos_t dpos);
  virtual std::shared_ptr<program::Expression> global_name(ExpressionCompiler & ec, int offset, const diagnostic::pos_t dpos);
  virtual std::shared_ptr<program::Expression> local_name(ExpressionCompiler & ec, int offset, const diagnostic::pos_t dpos);
  virtual std::shared_ptr<program::Expression> capture_name(ExpressionCompiler & ec, int offset, const diagnostic::pos_t dpos);

  VariableAccessor & operator=(const VariableAccessor &) = delete;

protected:
  std::shared_ptr<program::Expression> member_access(ExpressionCompiler & ec, const std::shared_ptr<program::Expression> & object, const int index, const diagnostic::pos_t dpos);
  std::shared_ptr<program::Expression> implicit_object(ExpressionCompiler & ec) const;
};

class ExpressionCompiler
{
private:
  Scope scope_;
  Function caller_; 
  
private:
  TypeResolver<BasicNameResolver> type_resolver;

  LambdaProcessor default_lambda_;
  LambdaProcessor *lambda_;

  VariableAccessor default_variable_;
  VariableAccessor *variable_;

  FunctionTemplateProcessor default_templates_;
  FunctionTemplateProcessor *templates_;

private:
  friend class VariableAccessor;

public:
  ExpressionCompiler();
  ExpressionCompiler(const Scope & scp);

  inline const Scope & scope() const { return scope_; }
  inline void setScope(const Scope & scp) { scope_ = scp; }

  inline const Function & caller() const { return caller_; }
  inline void setCaller(const Function & func) { caller_ = func; }
  
  inline Engine* engine() const { return scope_.engine(); }

  inline LambdaProcessor & lambdaProcessor() { return *lambda_; }
  inline void setLambdaProcessor(LambdaProcessor & lp) { lambda_ = &lp; }

  inline VariableAccessor & variableAccessor() { return *variable_; }
  inline void setVariableAccessor(VariableAccessor & va) { variable_ = &va; }

  inline FunctionTemplateProcessor & templateProcessor() { return *templates_; }
  inline void setTemplateProcessor(FunctionTemplateProcessor & ftp) { templates_ = &ftp; }

  std::shared_ptr<program::Expression> generateExpression(const std::shared_ptr<ast::Expression> & expr);
  std::vector<std::shared_ptr<program::Expression>> generateExpressions(const std::vector<std::shared_ptr<ast::Expression>> & expressions);
  void generateExpressions(const std::vector<std::shared_ptr<ast::Expression>> & in, std::vector<std::shared_ptr<program::Expression>> & out);

  std::shared_ptr<program::Expression> implicit_object() const;

protected:
  // diagnostics related (hopefully this is temporary)
  std::string dstr(const Type & t) const;

protected:

  NameLookup resolve(const std::shared_ptr<ast::Identifier> & identifier);

  virtual std::shared_ptr<program::Expression> generateOperation(const std::shared_ptr<ast::Expression> & op);

protected:
  Type resolve(const ast::QualifiedType & qt);

  std::vector<Function> getBinaryOperators(Operator::BuiltInOperator op, Type a, Type b);
  std::vector<Function> getUnaryOperators(Operator::BuiltInOperator op, Type a);
  std::vector<Function> getCallOperator(const Type & functor_type);
  std::vector<Function> getLiteralOperators(const std::string & suffix);

  std::shared_ptr<program::Expression> generateArrayConstruction(const std::shared_ptr<ast::ArrayExpression> & array_expr);
  std::shared_ptr<program::Expression> generateBraceConstruction(const std::shared_ptr<ast::BraceConstruction> & bc);
  std::shared_ptr<program::Expression> generateCall(const std::shared_ptr<ast::FunctionCall> & call);
  std::shared_ptr<program::Expression> generateConstructorCall(const std::shared_ptr<ast::FunctionCall> & fc, const Type & type, std::vector<std::shared_ptr<program::Expression>> && args);
  std::shared_ptr<program::Expression> generateListExpression(const std::shared_ptr<ast::ListExpression> & list_expr);
  std::shared_ptr<program::Expression> generateArraySubscript(const std::shared_ptr<ast::ArraySubscript> & as);
  std::shared_ptr<program::Expression> generateVirtualCall(const std::shared_ptr<ast::FunctionCall> & call, const Function & f, std::vector<std::shared_ptr<program::Expression>> && args);
  std::shared_ptr<program::Expression> generateFunctorCall(const std::shared_ptr<ast::FunctionCall> & call, const std::shared_ptr<program::Expression> & functor, std::vector<std::shared_ptr<program::Expression>> && args);
  std::shared_ptr<program::Expression> generateFunctionVariableCall(const std::shared_ptr<ast::FunctionCall> & call, const std::shared_ptr<program::Expression> & functor, std::vector<std::shared_ptr<program::Expression>> && args);
  std::shared_ptr<program::Expression> generateUserDefinedLiteral(const std::shared_ptr<ast::UserDefinedLiteral> & udl);
  std::shared_ptr<program::LambdaExpression> generateLambdaExpression(const std::shared_ptr<ast::LambdaExpression> & lambda_expr);
  std::shared_ptr<program::Expression> generateLiteral(const std::shared_ptr<ast::Literal> & literalExpr);
  std::shared_ptr<program::Expression> generateMemberAccess(const std::shared_ptr<ast::Operation> & operation);
  std::shared_ptr<program::Expression> generateBinaryOperation(const std::shared_ptr<ast::Operation> & operation);
  std::shared_ptr<program::Expression> generateUnaryOperation(const std::shared_ptr<ast::Operation> & operation);
  std::shared_ptr<program::Expression> generateConditionalExpression(const std::shared_ptr<ast::ConditionalExpression> & ce);
  std::shared_ptr<program::Expression> generateVariableAccess(const std::shared_ptr<ast::Identifier> & identifier);
  std::shared_ptr<program::Expression> generateVariableAccess(const std::shared_ptr<ast::Identifier> & identifier, const NameLookup & lookup);
  std::shared_ptr<program::Expression> generateFunctionAccess(const std::shared_ptr<ast::Identifier> & identifier, const NameLookup & lookup);
  std::shared_ptr<program::Expression> generateMemberAccess(const std::shared_ptr<program::Expression> & object, const int index, const diagnostic::pos_t dpos);
  std::shared_ptr<program::Expression> generateStaticDataMemberAccess(const std::shared_ptr<ast::Identifier> & id, const NameLookup & lookup);
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILE_EXPRESSION_H
