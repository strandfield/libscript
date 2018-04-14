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

void LambdaCompiler::preprocess(CompileLambdaTask & task, AbstractExpressionCompiler *c, const Stack & stack, int first_capture_offset)
{
  auto can_use_this = [&stack, first_capture_offset]() -> bool {
    return stack.size > first_capture_offset && stack.at(first_capture_offset).name == "this";
  };

  // fetching all captures
  const parser::Token capture_all_by_value = LambdaCompiler::captureAllByValue(*task.lexpr);
  const parser::Token capture_all_by_reference = LambdaCompiler::captureAllByReference(*task.lexpr);
  parser::Token catpure_this = LambdaCompiler::captureThis(*task.lexpr);
  if (!catpure_this.isValid() && can_use_this())
  {
    if (capture_all_by_reference.isValid())
      catpure_this = capture_all_by_reference;
    else
      catpure_this = capture_all_by_value;
  }

  if (capture_all_by_reference.isValid() && capture_all_by_value.isValid())
    throw CannotCaptureByValueAndByRef{ dpos(capture_all_by_reference) };

  std::vector<bool> capture_flags(stack.size, false);
  std::vector<Capture> captures;

  Class captured_class;
  if (catpure_this.isValid())
  {
    if (!can_use_this())
      throw CannotCaptureThis{ dpos(catpure_this) };

    std::shared_ptr<program::Expression> this_object = program::StackValue::New(first_capture_offset, stack.at(first_capture_offset).type);
    capture_flags[first_capture_offset] = true;

    captures.push_back(Capture{ "this", this_object });
    captured_class = c->engine()->getClass(this_object->type());
  }

  for (const auto & cap : task.lexpr->captures)
  {
    if (cap.byValueSign.isValid() || (cap.reference.isValid() && !cap.name.isValid()))
      continue;

    const auto & name = task.lexpr->captureName(cap);
    std::shared_ptr<program::Expression> value;
    if (cap.value != nullptr)
      value = c->generateExpression(cap.value);
    else
    {
      const int offset = stack.lastIndexOf(name);
      if (offset == -1)
        throw UnknownCaptureName{ dpos(cap.name) };
      value = program::StackValue::New(offset, stack.at(offset).type);
      if (!cap.reference.isValid())
      {
        StandardConversion conv = StandardConversion::compute(value->type(), value->type().baseType(), c->engine());
        if (conv == StandardConversion::NotConvertible())
          throw CannotCaptureNonCopyable{ dpos(cap.name) };
        value = c->applyStandardConversion(value, value->type().baseType(), conv);
      }

      capture_flags[offset] = true;
    }

    captures.push_back(Capture{ name, value });
  }

  /// TODO : what about the capture of this ?

  if (capture_all_by_value.isValid())
  {
    for (int i(first_capture_offset); i < stack.size; ++i)
    {
      if (capture_flags[i])
        continue;
      std::shared_ptr<program::Expression> value = program::StackValue::New(i, stack.at(i).type);
      StandardConversion conv = StandardConversion::compute(value->type(), value->type().baseType(), c->engine());
      if (conv == StandardConversion::NotConvertible())
        throw SomeLocalsCannotBeCaptured{ dpos(capture_all_by_value) };
      value = c->applyStandardConversion(value, value->type().baseType(), conv);
      captures.push_back(Capture{ stack.at(i).name, value });
    }
  }
  else if (capture_all_by_reference.isValid())
  {
    for (int i(first_capture_offset); i < stack.size; ++i)
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

  mLambda = build(Lambda{});

  for (const auto & cap : task.captures)
  {
    mLambda.impl()->captures.push_back(Lambda::Capture{ cap.type, cap.name });
  }

  mCurrentScope = Scope{ std::make_shared<LambdaScope>(mLambda, mCurrentScope.impl()) };

  const Prototype proto = computePrototype();
  FunctionBuilder builder = FunctionBuilder::Operator(Operator::FunctionCallOperator, proto);
  Operator function = build(builder).toOperator();

  mLambda.impl()->function = function;
  mFunction = function;

  /// TODO : where is the return value ?
  EnterScope guard{ this, FunctionScope::FunctionArguments };
  mStack.addVar(proto.argv(0), "lambda-object");
  for (int i(1); i < proto.argc(); ++i)
    std::dynamic_pointer_cast<FunctionScope>(mCurrentScope.impl())->add_var(task.lexpr->parameterName(i - 1), proto.argv(i));

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

  guard.leave(); // leaves the FunctionScope::FunctionArguments scope

  deduceReturnType(nullptr, nullptr); // deduces void if not already set

  function.implementation()->set_impl(body);

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
  /// TODO : remove unused captures
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
    return AbstractExpressionCompiler::generateMemberAccess(this_object, lookup.dataMemberIndex(), dpos(identifier));
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
    throw CannotDeduceLambdaReturnType{ dpos(rs) };

  mFunction.implementation()->prototype.setReturnType(val->type().baseType());
}

std::shared_ptr<program::Statement> LambdaCompiler::generateReturnStatement(const std::shared_ptr<ast::ReturnStatement> & rs)
{
  std::vector<std::shared_ptr<program::Statement>> statements;

  generateExitScope(mFunctionBodyScope, statements);

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
      throw InvalidUseOfDefaultArgument{ dpos(lexpr->params.at(i).defaultValue) };
    result.addArgument(paramtype);
  }

  return result;
}

} // namespace compiler

} // namespace script

