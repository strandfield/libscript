// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/lambdacompiler.h"

#include "script/compiler/compilererrors.h"
#include "script/compiler/conversionprocessor.h"
#include "script/compiler/defaultargumentprocessor.h"

#include "script/ast/node.h"

#include "script/program/expression.h"
#include "script/program/statements.h"

#include "script/functionbuilder.h"
#include "script/private/lambda_p.h"
#include "script/namelookup.h"
#include "script/private/namelookup_p.h"
#include "script/private/operator_p.h"


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


LambdaCompilerVariableAccessor::LambdaCompilerVariableAccessor(Stack & s, LambdaCompiler* fc)
  : StackVariableAccessor(s, fc) { }

std::shared_ptr<program::Expression> LambdaCompilerVariableAccessor::capture_name(ExpressionCompiler & ec, int offset, const diagnostic::pos_t dpos)
{
  LambdaCompiler *lc = static_cast<LambdaCompiler*>(fcomp_);
  auto lambda = program::StackValue::New(1, Type::ref(lc->mLambda.id()));
  const auto & capture = lc->task().captures[offset];
  auto capaccess = program::CaptureAccess::New(capture.type, lambda, offset);
  generated_access_.push_back(capaccess);
  return capaccess;
}

std::shared_ptr<program::Expression> LambdaCompilerVariableAccessor::data_member(ExpressionCompiler & ec, int offset, const diagnostic::pos_t dpos)
{
  LambdaCompiler *lc = static_cast<LambdaCompiler*>(fcomp_);
  auto lambda = program::StackValue::New(1, Type::ref(lc->mLambda.id()));
  auto this_object = program::CaptureAccess::New(Type::ref(lc->task().capturedObject.id()), lambda, 0);
  return member_access(ec, this_object, offset, dpos);
}



LambdaCompiler::LambdaCompiler(const std::shared_ptr<CompileSession> & s)
  : FunctionCompiler(s)
  , mCurrentTask(nullptr)
  , variable_(mStack, this)
{
  expr_.setVariableAccessor(this->variable_);
}

void LambdaCompiler::preprocess(CompileLambdaTask & task, ExpressionCompiler *c, const Stack & stack, int first_capture_offset)
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
        value = ConversionProcessor::sconvert(c->engine(), value, value->type().baseType(), conv);
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
      value = ConversionProcessor::sconvert(c->engine(), value, value->type().baseType(), conv);
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
  /// TODO : we should set mDeclaration, but FunctionCompiler assumes it's a ast::FunctionDecl
  // we should fix that !!
  //mDeclaration = task.lexpr;

  mLambda = newLambda();

  for (const auto & cap : task.captures)
  {
    mLambda.impl()->captures.push_back(ClosureType::Capture{ cap.type, cap.name });
  }

  mCurrentScope = Scope{ std::make_shared<LambdaScope>(mLambda, mCurrentScope.impl()) };

  const Prototype proto = computePrototype();
  FunctionBuilder builder = FunctionBuilder::Operator(Operator::FunctionCallOperator, proto);
  Operator function = build(builder).toOperator();

  mLambda.impl()->function = function;
  mFunction = function;

  /// TODO : where is the return value ?
  EnterScope guard{ this, FunctionScope::FunctionArguments };
  mStack.addVar(proto.at(0), "lambda-object");
  for (int i(1); i < proto.count(); ++i)
    std::dynamic_pointer_cast<FunctionScope>(mCurrentScope.impl())->add_var(task.lexpr->parameterName(i - 1), proto.at(i));

  std::shared_ptr<program::CompoundStatement> body = this->generateCompoundStatement(task.lexpr->body, FunctionScope::FunctionBody);

  guard.leave(); // leaves the FunctionScope::FunctionArguments scope

  deduceReturnType(nullptr, nullptr); // deduces void if not already set

  function.impl()->set_impl(body);

  DefaultArgumentProcessor default_arguments;
  default_arguments.process(task.lexpr->params, function, task.scope);

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

void LambdaCompiler::deduceReturnType(const std::shared_ptr<ast::ReturnStatement> & rs, const std::shared_ptr<program::Expression> & val)
{
  if (mFunction.returnType() != Type::Null)
    return;

  if (val == nullptr)
  {
    mFunction.impl()->prototype.setReturnType(Type::Void);
    return;
  }

  if (val->type() == Type::InitializerList)
    throw CannotDeduceLambdaReturnType{ dpos(rs) };

  mFunction.impl()->prototype.setReturnType(val->type().baseType());
}

void LambdaCompiler::processReturnStatement(const std::shared_ptr<ast::ReturnStatement> & rs)
{
  std::vector<std::shared_ptr<program::Statement>> statements;

  generateExitScope(mFunctionBodyScope, statements);

  std::shared_ptr<program::Expression> retval = rs->expression != nullptr ? generate(rs->expression) : nullptr;
  deduceReturnType(rs, retval);

  if (rs->expression == nullptr)
  {
    if (mFunction.prototype().returnType() != Type::Void)
      throw ReturnStatementWithoutValue{};

    return write(program::ReturnStatement::New(nullptr, std::move(statements)));
  }
  else
  {
    if (mFunction.prototype().returnType() == Type::Void)
      throw ReturnStatementWithValue{};
  }

  const ConversionSequence conv = ConversionSequence::compute(retval, mFunction.prototype().returnType(), engine());

  /// TODO : write a dedicated function for this, don't use prepareFunctionArg()
  retval = ConversionProcessor::convert(engine(), retval, mFunction.prototype().returnType(), conv);

  write(program::ReturnStatement::New(retval, std::move(statements)));
}

Prototype LambdaCompiler::computePrototype()
{
  const auto & lexpr = task().lexpr;

  Prototype result;

  // Warning : return type is set later
  result.setReturnType(Type::Null);

  Type closure_type{ mLambda.id(), Type::ReferenceFlag | Type::ThisFlag };
  result.addParameter(closure_type);

  for (size_t i(0); i < lexpr->params.size(); ++i)
  {
    Type paramtype = type_.resolve(lexpr->params.at(i).type, mCurrentScope);
    result.addParameter(paramtype);
  }

  return result;
}

} // namespace compiler

} // namespace script

