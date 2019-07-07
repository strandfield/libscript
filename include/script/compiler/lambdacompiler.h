// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILE_LAMBDA_H
#define LIBSCRIPT_COMPILE_LAMBDA_H

#include "script/compiler/functioncompiler.h"

namespace script
{

namespace parser
{
class Token;
}

namespace program
{
class CaptureAccess;
} // namespace program

namespace compiler
{

class Capture
{
public:
  Capture(const std::string & n, const std::shared_ptr<program::Expression> & value);
  Capture(const Capture &) = default;
  ~Capture() = default;

  Capture & operator=(const Capture &) = default;

public:
  Type type;
  std::string name;
  std::shared_ptr<program::Expression> value;
  bool used;
};

struct CompileLambdaTask
{
  std::shared_ptr<ast::LambdaExpression> lexpr;
  std::vector<Capture> captures;
  Class capturedObject; /// TODO : shouldn't be this also a Capture ?
  script::Scope scope;
};

struct LambdaCompilationResult
{
  std::shared_ptr<program::LambdaExpression> expression;
  ClosureType closure_type;
};

class LambdaCompiler;

class LambdaCompiler : public FunctionCompiler
{
public:
  explicit LambdaCompiler(Engine *e);

  static void preprocess(CompileLambdaTask & task, ExpressionCompiler *c, const Stack & stack, int first_capture_offset);

  LambdaCompilationResult compile(const CompileLambdaTask & task);
  const CompileLambdaTask & task() const;

  bool thisCaptured() const;

  static parser::Token captureAllByValue(const ast::LambdaExpression & lexpr);
  static parser::Token captureAllByReference(const ast::LambdaExpression & lexpr);
  static parser::Token captureThis(const ast::LambdaExpression & lexpr);

protected:
  void removeUnusedCaptures();

protected:
  using FunctionCompiler::resolve;

  void deduceReturnType(const std::shared_ptr<ast::ReturnStatement> & rs, const std::shared_ptr<program::Expression> & val);
  void processReturnStatement(const std::shared_ptr<ast::ReturnStatement> & rs) override;

protected:
  DynamicPrototype computePrototype();

protected:
  CompileLambdaTask const *mCurrentTask;

private:
  std::vector<Capture> mCaptures;
  ClosureType mLambda;
};


} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILE_FUNCTION_H
