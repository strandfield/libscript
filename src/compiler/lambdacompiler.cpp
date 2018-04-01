// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/lambdacompiler.h"

#include "script/compiler/compilererrors.h"

#include "script/ast/node.h"

#include "script/program/expression.h"
#include "script/program/statements.h"

#include "script/functionbuilder.h"
#include "../lambda_p.h"
#include "script/namelookup.h"
#include "../namelookup_p.h"
#include "../operator_p.h"


namespace script
{

namespace compiler
{

Capture::Capture(const std::string & n, const std::shared_ptr<program::Expression> & val)
  : type(val->type())
  , name(n)
  , value(val)
  , used(false)
{

}


LambdaCompiler::LambdaCompiler(Compiler *c, CompileSession *s)
  : FunctionCompiler(c, s)
  , mCurrentTask(nullptr)
{

}

LambdaCompilationResult LambdaCompiler::compile(const CompileLambdaTask & task)
{
  mCurrentTask = &task;
  mCurrentScope = task.scope;

  mLambda = build(Lambda{});

  const Prototype proto = computePrototype();
  FunctionBuilder builder = FunctionBuilder::Operator(Operator::FunctionCallOperator, proto);
  Operator function = build(builder).toOperator();

  mLambda.impl()->function = function;
  mFunction = function;

  mStack.addVar(proto.argv(0), "lambda-object");
  for (int i(1); i < proto.argc(); ++i)
    mStack.addVar(proto.argv(i), task.lexpr->parameterName(i-1));

  std::shared_ptr<program::CompoundStatement> body = this->generateCompoundStatement(task.lexpr->body, FunctionScope::FunctionBody);

  // generating default arguments
  std::vector<std::shared_ptr<program::Statement>> default_arg_inits;
  if (function.prototype().hasDefaultArgument())
  {
    const int default_arg_count = function.prototype().defaultArgCount();
    for (int i(function.prototype().argc() - default_arg_count); i < function.prototype().argc(); ++i)
    {
      auto val = generateDefaultArgument(i);
      default_arg_inits.push_back(program::PushDefaultArgument::New(i, val));
    }
  }
  body->statements.insert(body->statements.begin(), default_arg_inits.begin(), default_arg_inits.end());

  deduceReturnType(nullptr, nullptr); // deduces void if not already set

  function.implementation()->set_impl(body);

  removeUnusedCaptures();

  /// TODO : what if this is captured ?
  auto lexpr = program::LambdaExpression::New(Type{ mLambda.id() }, {});
  for (const auto & cap : mCaptures)
  {
    lexpr->captures.push_back(cap.value);
    mLambda.impl()->captures.push_back(Lambda::Capture{cap.type, cap.name});
  }

  return { lexpr, mLambda };
}

const CompileLambdaTask & LambdaCompiler::task() const
{
  return *mCurrentTask;
}

bool LambdaCompiler::thisCaptured() const
{
  return !task().capturedObject.isNull();
}

parser::Token LambdaCompiler::captureAllByValue(const ast::LambdaExpression & lexpr)
{
  for (const auto & cap : lexpr.captures)
  {
    if (cap.byValueSign.isValid())
      return cap.byValueSign;
  }

  return parser::Token{};
}

parser::Token LambdaCompiler::captureAllByReference(const ast::LambdaExpression & lexpr)
{
  for (const auto & cap : lexpr.captures)
  {
    if (cap.reference.isValid() && !cap.name.isValid())
      return cap.reference;
  }

  return parser::Token{};
}

parser::Token LambdaCompiler::captureThis(const ast::LambdaExpression & lexpr)
{
  for (const auto & cap : lexpr.captures)
  {
    if (cap.name == parser::Token::This)
      return cap.name;
  }

  return parser::Token{};
}


void LambdaCompiler::removeUnusedCaptures()
{
  mCaptures = task().captures;
  /// TODO : remove unused captures
}

NameLookup LambdaCompiler::unqualifiedLookup(const std::shared_ptr<ast::Identifier> & name)
{
  assert(name->type() == ast::NodeType::SimpleIdentifier);

  if (name->name == parser::Token::This)
  {
    if (!this->thisCaptured())
      throw IllegalUseOfThis{ name->name.line, name->name.column };

    auto result = std::make_shared<NameLookupImpl>();
    result->captureIndex = 0;
    return NameLookup{ result };
  }

  const Stack & stack = mStack;
  const std::string & str = name->getName();
  const int offset = stack.lastIndexOf(str);
  if (offset != -1)
  {
    auto result = std::make_shared<NameLookupImpl>();
    result->localIndex = offset;
    return NameLookup{ result };
  }

  const auto & captures = task().captures;
  for (int i(captures.size() - 1); i >= 0; --i)
  {
    if (captures.at(i).name == str)
    {
      auto result = std::make_shared<NameLookupImpl>();
      result->captureIndex = i + (thisCaptured() ? 1 : 0);
      return NameLookup{ result };
    }
  }

  if (thisCaptured())
  {
    Class cla = task().capturedObject;
    const int dmi = cla.attributeIndex(str);
    if (dmi != -1)
    {
      auto result = std::make_shared<NameLookupImpl>();
      result->dataMemberIndex = dmi;
      return NameLookup{ result };
    }
  }

  return NameLookup::resolve(name, currentScope());
}


std::shared_ptr<program::Expression> LambdaCompiler::generateVariableAccess(const std::shared_ptr<ast::Identifier> & identifier, const NameLookup & lookup)
{
  if (lookup.resultType() == NameLookup::CaptureName)
  {
    auto lambda = program::StackValue::New(1, Type::ref(mLambda.id()));
    const auto & capture = task().captures[lookup.captureIndex()];
    auto capaccess = program::CaptureAccess::New(capture.type, lambda, lookup.captureIndex());
    mCaptureAccess.push_back(capaccess);
    return capaccess;
  }
  else if (lookup.resultType() == NameLookup::DataMemberName)
  {
    auto lambda = program::StackValue::New(1, Type::ref(mLambda.id()));
    auto this_object = program::CaptureAccess::New(Type::ref(task().capturedObject.id()), lambda, 0);
    return generateMemberAccess(this_object, lookup.dataMemberIndex());
  }

  return FunctionCompiler::generateVariableAccess(identifier, lookup);
}

void LambdaCompiler::deduceReturnType(const std::shared_ptr<ast::ReturnStatement> & rs, const std::shared_ptr<program::Expression> & val)
{
  if (mFunction.returnType() != Type::Null)
    return;

  if (val == nullptr)
  {
    mFunction.implementation()->prototype.setReturnType(Type::Void);
    return;
  }

  if (val->type() == Type::InitializerList)
    throw CannotDeduceLambdaReturnType{rs->pos().line, rs->pos().col};

  mFunction.implementation()->prototype.setReturnType(val->type().baseType());
}

std::shared_ptr<program::Statement> LambdaCompiler::generateReturnStatement(const std::shared_ptr<ast::ReturnStatement> & rs)
{
  std::vector<std::shared_ptr<program::Statement>> statements;

  const int depth = mScopeStack.size();
  generateScopeDestruction(depth, statements);

  std::shared_ptr<program::Expression> retval = rs->expression != nullptr ? generateExpression(rs->expression) : nullptr;
  deduceReturnType(rs, retval);

  if (rs->expression == nullptr)
  {
    if (mFunction.prototype().returnType() != Type::Void)
      throw ReturnStatementWithoutValue{};

    return program::ReturnStatement::New(nullptr, std::move(statements));
  }
  else
  {
    if (mFunction.prototype().returnType() == Type::Void)
      throw ReturnStatementWithValue{};
  }

  const ConversionSequence conv = ConversionSequence::compute(retval, mFunction.prototype().returnType(), engine());

  /// TODO : write a dedicated function for this, don't use prepareFunctionArg()
  retval = prepareFunctionArgument(retval, mFunction.prototype().returnType(), conv);

  return program::ReturnStatement::New(retval, std::move(statements));
}

Prototype LambdaCompiler::computePrototype()
{
  const auto & lexpr = task().lexpr;

  Prototype result;

  // Warning : return type is set later
  result.setReturnType(Type::Null);

  Type closure_type{ mLambda.id(), Type::ReferenceFlag | Type::ThisFlag };
  result.addArgument(closure_type);

  bool mustbe_defaulted = false;
  for (size_t i(0); i < lexpr->params.size(); ++i)
  {
    Type paramtype = resolve(lexpr->params.at(i).type);
    if (lexpr->params.at(i).defaultValue != nullptr)
      paramtype.setFlag(Type::OptionalFlag), mustbe_defaulted = true;
    else if (mustbe_defaulted)
      throw InvalidUseOfDefaultArgument{ lexpr->params.at(i).defaultValue->pos().line, lexpr->params.at(i).defaultValue->pos().col };
    result.addArgument(paramtype);
  }

  return result;
}

} // namespace compiler

} // namespace script

