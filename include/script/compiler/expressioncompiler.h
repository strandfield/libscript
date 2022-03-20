// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILE_EXPRESSION_H
#define LIBSCRIPT_COMPILE_EXPRESSION_H

#include "script/compiler/component.h"

#include "script/compiler/nameresolver.h"
#include "script/compiler/typeresolver.h"

#include "script/compiler/variableaccessor.h"

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
class FunctionCompiler;
class Logger;

class LIBSCRIPT_API LambdaProcessor : public Component
{
private:
  Stack* stack_ = nullptr;

public:
  LambdaProcessor(Compiler* c);
  LambdaProcessor(const LambdaProcessor &) = delete;
  ~LambdaProcessor() = default;

  bool allowCaptures() const { return stack_ != nullptr; }

  void setStack(Stack* s);

  std::shared_ptr<program::LambdaExpression> generate(ExpressionCompiler & ec, const std::shared_ptr<ast::LambdaExpression> & le);

  LambdaProcessor & operator=(const LambdaProcessor &) = delete;
};

class LIBSCRIPT_API ExpressionCompiler : public Component
{
private:
  Scope scope_;
  Function caller_;
  Stack* stack_ = nullptr;

private:
  VariableAccessor variables_;

  FunctionTemplateProcessor templates_;

  std::shared_ptr<program::Expression> implicit_object_;

public:
  explicit ExpressionCompiler(Compiler *c);
  ExpressionCompiler(Compiler* c, const Scope & scp);

  inline const Scope & scope() const { return scope_; }
  void setScope(const Scope& scp);

  inline const Function & caller() const { return caller_; }
  void setCaller(const Function & func);

  void setStack(Stack* s);
  
  VariableAccessor& variableAccessor() { return variables_; }

  std::shared_ptr<program::Expression> generateExpression(const std::shared_ptr<ast::Expression> & expr);
  std::vector<std::shared_ptr<program::Expression>> generateExpressions(const std::vector<std::shared_ptr<ast::Expression>> & expressions);
  void generateExpressions(const std::vector<std::shared_ptr<ast::Expression>> & in, std::vector<std::shared_ptr<program::Expression>> & out);

  inline const std::shared_ptr<program::Expression> & implicit_object() const { return implicit_object_; }

protected:

  NameLookup resolve(const std::shared_ptr<ast::Identifier> & identifier);

  static std::vector<Type> getTypes(const std::vector<std::shared_ptr<program::Expression>>& exprs);
  static const std::vector<std::shared_ptr<ast::Node>>& getTemplateArgs(const std::shared_ptr<ast::Identifier>& id);

  virtual std::shared_ptr<program::Expression> generateOperation(const std::shared_ptr<ast::Expression> & op);
  static void complete(const Function & f, std::vector<std::shared_ptr<program::Expression>> & args);

protected:
  std::vector<Function> getBinaryOperators(OperatorName op, Type a, Type b);
  std::vector<Function> getUnaryOperators(OperatorName op, Type a);
  std::vector<Function> getCallOperator(const Type & functor_type);
  std::vector<Function> getLiteralOperators(const std::string & suffix);

  std::shared_ptr<program::Expression> generateArrayConstruction(const std::shared_ptr<ast::ArrayExpression> & array_expr);
  std::shared_ptr<program::Expression> generateBraceConstruction(const std::shared_ptr<ast::BraceConstruction> & bc);
  std::shared_ptr<program::Expression> generateCall(const std::shared_ptr<ast::FunctionCall> & call);
  std::shared_ptr<program::Expression> generateCall(const std::shared_ptr<ast::FunctionCall> & call, const std::shared_ptr<ast::Identifier> callee_name, const std::shared_ptr<program::Expression> & object, std::vector<std::shared_ptr<program::Expression>> & args, const NameLookup & lookup);
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
  std::shared_ptr<program::Expression> generateStaticDataMemberAccess(const std::shared_ptr<ast::Identifier> & id, const NameLookup & lookup);
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILE_EXPRESSION_H
