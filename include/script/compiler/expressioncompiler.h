// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILE_EXPRESSION_H
#define LIBSCRIPT_COMPILE_EXPRESSION_H

#include "script/compiler/compiler.h"

#include "script/context.h"
#include "script/conversions.h"

#include "script/ast/forwards.h"

namespace script
{

class NameLookup;
class Template;
struct TemplateArgument;

namespace program
{
class Expression;
class LambdaExpression;
} // namespace program

namespace compiler
{

class AbstractExpressionCompiler : public CompilerComponent
{
public:
  AbstractExpressionCompiler() = delete;
  AbstractExpressionCompiler(Compiler *c, CompileSession *s);

  TemplateArgument generateTemplateArgument(const std::shared_ptr<ast::Node> & arg);
  TemplateArgument generateTemplateArgument(const std::shared_ptr<program::Expression> & e, const std::shared_ptr<ast::Node> &);
  std::vector<TemplateArgument> generateTemplateArguments(const std::vector<std::shared_ptr<ast::Node>> & args);
  bool isConstExpr(const std::shared_ptr<program::Expression> & expr);
  Value evalConstExpr(const std::shared_ptr<program::Expression> & expr);

  virtual Scope scope() const = 0;
  virtual Function caller() const { return Function{}; }

protected:
  friend class ConstructorCompiler;
  friend class LambdaCompiler;
  friend class ScriptCompiler;

  NameLookup resolve(const std::shared_ptr<ast::Identifier> & identifier);

  virtual std::shared_ptr<program::Expression> generateVariableAccess(const std::shared_ptr<ast::Identifier> & identifier, const NameLookup & lookup) = 0;

  virtual std::shared_ptr<program::Expression> generateOperation(const std::shared_ptr<ast::Expression> & op);
  virtual std::shared_ptr<program::Expression> generateCall(const std::shared_ptr<ast::FunctionCall> & call);

  virtual std::shared_ptr<program::LambdaExpression> generateLambdaExpression(const std::shared_ptr<ast::LambdaExpression> & lambda_expr) = 0;

  virtual std::string repr(const std::shared_ptr<ast::Identifier> & id);

  std::shared_ptr<program::Expression> generateExpression(const std::shared_ptr<ast::Expression> & expr);


protected:
  Type resolveFunctionType(const ast::QualifiedType & qt);
  Type resolve(const ast::QualifiedType & qt);

  std::vector<Function> getBinaryOperators(Operator::BuiltInOperator op, Type a, Type b);
  std::vector<Function> getUnaryOperators(Operator::BuiltInOperator op, Type a);
  std::vector<Function> getCallOperator(const Type & functor_type);
  std::vector<Function> getLiteralOperators(const std::string & suffix);


  std::vector<std::shared_ptr<program::Expression>> generateExpressions(const std::vector<std::shared_ptr<ast::Expression>> & expressions);
  void generateExpressions(const std::vector<std::shared_ptr<ast::Expression>> & in, std::vector<std::shared_ptr<program::Expression>> & out);

  std::shared_ptr<program::Expression> applyStandardConversion(const std::shared_ptr<program::Expression> & arg, const Type & type, const StandardConversion & conv);
  std::shared_ptr<program::Expression> performListInitialization(const std::shared_ptr<program::Expression> & arg, const Type & type, const std::shared_ptr<ListInitializationSequence> & linit);
  std::shared_ptr<program::Expression> prepareFunctionArgument(const std::shared_ptr<program::Expression> & arg, const Type & type, const ConversionSequence & conv);
  void prepareFunctionArguments(std::vector<std::shared_ptr<program::Expression>> & args, const Prototype & proto, const std::vector<ConversionSequence> & conversions);


  std::shared_ptr<program::Expression> constructFundamentalValue(const Type & t, bool copy);
  std::shared_ptr<program::Expression> braceConstructValue(const Type & t, std::vector<std::shared_ptr<program::Expression>> && args, diagnostic::pos_t dp);
  std::shared_ptr<program::Expression> constructValue(const Type & t, std::nullptr_t, diagnostic::pos_t dp);
  std::shared_ptr<program::Expression> constructValue(const Type & t, std::vector<std::shared_ptr<program::Expression>> && args, diagnostic::pos_t dp);
  std::shared_ptr<program::Expression> constructValue(const Type & t, const std::shared_ptr<ast::ConstructorInitialization> & init);
  std::shared_ptr<program::Expression> constructValue(const Type & t, const std::shared_ptr<ast::BraceInitialization> & init);

  int generateIntegerLiteral(const std::shared_ptr<ast::IntegerLiteral> & l);

  std::shared_ptr<program::Expression> generateArrayConstruction(const std::shared_ptr<ast::ArrayExpression> & array_expr);
  std::shared_ptr<program::Expression> generateBraceConstruction(const std::shared_ptr<ast::BraceConstruction> & bc);
  std::shared_ptr<program::Expression> generateConstructorCall(const std::shared_ptr<ast::FunctionCall> & fc, const Type & type, std::vector<std::shared_ptr<program::Expression>> && args);
  std::shared_ptr<program::Expression> generateListExpression(const std::shared_ptr<ast::ListExpression> & list_expr);
  std::shared_ptr<program::Expression> generateArraySubscript(const std::shared_ptr<ast::ArraySubscript> & as);
  std::shared_ptr<program::Expression> generateVirtualCall(const std::shared_ptr<ast::FunctionCall> & call, const Function & f, std::vector<std::shared_ptr<program::Expression>> && args);
  std::shared_ptr<program::Expression> generateFunctorCall(const std::shared_ptr<ast::FunctionCall> & call, const std::shared_ptr<program::Expression> & functor, std::vector<std::shared_ptr<program::Expression>> && args);
  std::shared_ptr<program::Expression> generateFunctionVariableCall(const std::shared_ptr<ast::FunctionCall> & call, const std::shared_ptr<program::Expression> & functor, std::vector<std::shared_ptr<program::Expression>> && args);
  std::shared_ptr<program::Expression> generateUserDefinedLiteral(const std::shared_ptr<ast::UserDefinedLiteral> & udl);
  Value generateStringLiteral(const std::shared_ptr<ast::Literal> & l, std::string && str);
  std::shared_ptr<program::Expression> generateLiteral(const std::shared_ptr<ast::Literal> & literalExpr);
  std::shared_ptr<program::Expression> generateMemberAccess(const std::shared_ptr<ast::Operation> & operation);
  std::shared_ptr<program::Expression> generateBinaryOperation(const std::shared_ptr<ast::Operation> & operation);
  std::shared_ptr<program::Expression> generateUnaryOperation(const std::shared_ptr<ast::Operation> & operation);
  std::shared_ptr<program::Expression> generateConditionalExpression(const std::shared_ptr<ast::ConditionalExpression> & ce);
  std::shared_ptr<program::Expression> generateVariableAccess(const std::shared_ptr<ast::Identifier> & identifier);
  std::shared_ptr<program::Expression> generateFunctionAccess(const std::shared_ptr<ast::Identifier> & identifier, const NameLookup & lookup);
  std::shared_ptr<program::Expression> generateMemberAccess(const std::shared_ptr<program::Expression> & object, const int index, const diagnostic::pos_t dpos);
  std::shared_ptr<program::Expression> generateStaticDataMemberAccess(const std::shared_ptr<ast::Identifier> & id, const NameLookup & lookup);
};


class ExpressionCompiler : public AbstractExpressionCompiler
{
public:
  ExpressionCompiler(Compiler *c, CompileSession *s);

  void setScope(const Scope & s);

  std::shared_ptr<program::Expression> compile(const std::shared_ptr<ast::Expression> & expr, const Context & context);
  std::shared_ptr<program::Expression> compile(const std::shared_ptr<ast::Expression> & expr, const Scope & scp);

protected:
  Scope scope() const override;
  std::shared_ptr<program::Expression> generateOperation(const std::shared_ptr<ast::Expression> & op) override;
  std::shared_ptr<program::Expression> generateVariableAccess(const std::shared_ptr<ast::Identifier> & identifier, const NameLookup & lookup)  override;
  std::shared_ptr<program::LambdaExpression> generateLambdaExpression(const std::shared_ptr<ast::LambdaExpression> & lambda_expr) override;

private:
  Context mContext;
  Scope mScope;
};


} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILE_EXPRESSION_H
