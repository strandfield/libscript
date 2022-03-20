// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/lambdacompiler.h"

#include "script/compiler/compilererrors.h"
#include "script/compiler/compilesession.h"
#include "script/compiler/conversionprocessor.h"
#include "script/compiler/defaultargumentprocessor.h"

#include "script/ast/node.h"

#include "script/program/expression.h"
#include "script/program/statements.h"

#include "script/engine.h"
#include "script/functionbuilder.h"
#include "script/private/lambda_p.h"
#include "script/namelookup.h"
#include "script/typesystem.h"

#include "script/private/engine_p.h"
#include "script/private/namelookup_p.h"
#include "script/private/operator_p.h"
#include "script/private/typesystem_p.h"

namespace script
{

namespace compiler
{

Capture::Capture(utils::StringView n, const std::shared_ptr<program::Expression> & val)
  : type(val->type())
  , name(n)
  , value(val)
  , used(false)
{

}

LambdaCompiler::LambdaCompiler(Compiler* c)
  : FunctionCompiler(c)
  , mCurrentTask(nullptr)
{

}

void LambdaCompiler::preprocess(CompileLambdaTask & task, ExpressionCompiler *c, const Stack & stack, int first_capture_offset)
{
  auto can_use_this = [&stack, first_capture_offset]() -> bool {
    return stack.size() > first_capture_offset && stack.at(first_capture_offset).name == "this";
  };

  // fetching all captures
  const parser::Token capture_all_by_value = LambdaCompiler::captureAllByValue(*task.lexpr);
  const parser::Token capture_all_by_reference = LambdaCompiler::captureAllByReference(*task.lexpr);
  parser::Token capture_this = LambdaCompiler::captureThis(*task.lexpr);
  if (!capture_this.isValid() && can_use_this())
  {
    if (capture_all_by_reference.isValid())
      capture_this = capture_all_by_reference;
    else
      capture_this = capture_all_by_value;
  }

  if (capture_all_by_reference.isValid() && capture_all_by_value.isValid())
    throw CompilationFailure{ CompilerError::CannotCaptureByValueAndByRef };

  std::vector<bool> capture_flags(stack.size(), false);
  std::vector<Capture> captures;

  Class captured_class;
  if (capture_this.isValid())
  {
    TranslationTarget target{ c, capture_this };

    if (!can_use_this())
      throw CompilationFailure{ CompilerError::CannotCaptureThis };

    std::shared_ptr<program::Expression> this_object = program::StackValue::New(first_capture_offset, stack.at(first_capture_offset).type);
    capture_flags[first_capture_offset] = true;

    captures.push_back(Capture{ "this", this_object });
    captured_class = c->engine()->typeSystem()->getClass(this_object->type());
  }

  for (const auto & cap : task.lexpr->captures)
  {
    if (cap.byValueSign.isValid() || (cap.reference.isValid() && !cap.name.isValid()))
      continue;

    TranslationTarget target{ c, cap.name };

    const utils::StringView name = cap.name.text();
    std::shared_ptr<program::Expression> value;
    if (cap.value != nullptr)
    {
      value = c->generateExpression(cap.value);
    }
    else
    {
      const int offset = stack.lastIndexOf(name);

      if (offset == -1)
        throw CompilationFailure{ CompilerError::UnknownCaptureName };

      value = program::StackValue::New(offset, stack.at(offset).type);
      if (!cap.reference.isValid())
      {
        StandardConversion conv = StandardConversion::compute(value->type(), value->type().baseType(), c->engine());

        if (conv == StandardConversion::NotConvertible())
          throw CompilationFailure{ CompilerError::CannotCaptureNonCopyable };

        value = ConversionProcessor::sconvert(c->engine(), value, conv);
      }

      capture_flags[offset] = true;
    }

    captures.push_back(Capture{ name, value });
  }

  /// TODO : what about the capture of this ?

  if (capture_all_by_value.isValid())
  {
    TranslationTarget target{ c, capture_all_by_value };

    for (int i(first_capture_offset); i < stack.size(); ++i)
    {
      if (capture_flags[i])
        continue;
      std::shared_ptr<program::Expression> value = program::StackValue::New(i, stack.at(i).type);
      StandardConversion conv = StandardConversion::compute(value->type(), value->type().baseType(), c->engine());

      if (conv == StandardConversion::NotConvertible())
        throw CompilationFailure{ CompilerError::SomeLocalsCannotBeCaptured };

      value = ConversionProcessor::sconvert(c->engine(), value, conv);
      captures.push_back(Capture{ stack.at(i).name, value });
    }
  }
  else if (capture_all_by_reference.isValid())
  {
    for (int i(first_capture_offset); i < stack.size(); ++i)
    {
      if (capture_flags[i])
        continue;
      auto value = program::StackValue::New(i, stack.at(i).type);
      captures.push_back(Capture{ stack.at(i).name, value });
    }
  }


  task.captures = std::move(captures);
  task.capturedObject = captured_class;
}

LambdaCompilationResult LambdaCompiler::compile(const CompileLambdaTask & task)
{
  mCurrentTask = &task;
  mCurrentScope = task.scope;
  /// TODO : we should set mDeclaration, but FunctionCompiler assumes it's a ast::FunctionDecl
  // we should fix that !!
  //mDeclaration = task.lexpr;

  mLambda = engine()->typeSystem()->impl()->newLambda();

  for (const auto & cap : task.captures)
  {
    mLambda.impl()->captures.push_back(ClosureType::Capture{ cap.type, cap.name.toString() });
  }

  mCurrentScope = Scope{ std::make_shared<LambdaScope>(mLambda, mCurrentScope.impl()) };

  FunctionBuilder builder{ Symbol(Class(mLambda.impl())), SymbolKind::Operator, OperatorName::FunctionCallOperator };
  builder.blueprint_.prototype_ = computePrototype();
  DefaultArgumentProcessor default_arguments{ compiler() };
  default_arguments.generic_process(task.lexpr->params, builder, task.scope);

  Function function = builder.get();

  mFunction = function;
  expr_.setCaller(function);

  /// TODO : where is the return value ?
  EnterScope guard{ this, FunctionScope::FunctionArguments };
  mStack.addVar(function.prototype().at(0), "lambda-object");
  for (int i(1); i < function.prototype().count(); ++i)
    std::dynamic_pointer_cast<FunctionScope>(mCurrentScope.impl())->add_var(task.lexpr->parameterName(i - 1), function.parameter(i));

  std::shared_ptr<program::CompoundStatement> body = this->generateCompoundStatement(task.lexpr->body, FunctionScope::FunctionBody);

  guard.leave(); // leaves the FunctionScope::FunctionArguments scope

  deduceReturnType(nullptr, nullptr); // deduces void if not already set

  // @TODO: improve that, really ugly
  dynamic_cast<FunctionCallOperatorImpl*>(function.impl().get())->program_ = body;

  removeUnusedCaptures();

  /// TODO : what if this is captured ?
  auto lexpr = program::LambdaExpression::New(Type{ mLambda.id() }, {});
  for (const auto & cap : mCaptures)
  {
    lexpr->captures.push_back(cap.value);
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
  const auto& used = expr_.variableAccessor().generatedCaptures();
  /// TODO : remove unused captures
}

void LambdaCompiler::deduceReturnType(const std::shared_ptr<ast::ReturnStatement> & rs, const std::shared_ptr<program::Expression> & val)
{
  if (mFunction.returnType() != Type::Null)
    return;

  if (val == nullptr)
  {
    mFunction.impl()->set_return_type(Type::Void);
    return;
  }

  if (val->type() == Type::InitializerList)
    throw CompilationFailure{ CompilerError::CannotDeduceLambdaReturnType };

  mFunction.impl()->set_return_type(val->type().baseType());
}

void LambdaCompiler::processReturnStatement(const std::shared_ptr<ast::ReturnStatement> & rs)
{
  std::vector<std::shared_ptr<program::Statement>> statements;

  generateExitScope(mFunctionBodyScope, statements, *rs);

  std::shared_ptr<program::Expression> retval = rs->expression != nullptr ? generate(rs->expression) : nullptr;
  deduceReturnType(rs, retval);

  if (rs->expression == nullptr)
  {
    if (mFunction.prototype().returnType() != Type::Void)
      throw CompilationFailure{ CompilerError::ReturnStatementWithoutValue };

    return write(program::ReturnStatement::New(nullptr, std::move(statements)));
  }
  else
  {
    if (mFunction.prototype().returnType() == Type::Void)
      throw CompilationFailure{ CompilerError::ReturnStatementWithValue };
  }

  const Conversion conv = Conversion::compute(retval, mFunction.prototype().returnType(), engine());

  /// TODO : write a dedicated function for this, don't use prepareFunctionArg()
  retval = ConversionProcessor::convert(engine(), retval, conv);

  write(program::ReturnStatement::New(retval, std::move(statements)));
}

DynamicPrototype LambdaCompiler::computePrototype()
{
  const auto & lexpr = task().lexpr;

  DynamicPrototype result;

  // Warning : return type is set later
  result.setReturnType(Type::Null);

  Type closure_type{ mLambda.id(), Type::ReferenceFlag | Type::ThisFlag };
  result.push(closure_type);

  for (size_t i(0); i < lexpr->params.size(); ++i)
  {
    Type paramtype = script::compiler::resolve_type(lexpr->params.at(i).type, mCurrentScope);
    result.push(paramtype);
  }

  return result;
}

} // namespace compiler

} // namespace script

