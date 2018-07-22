// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILE_SESSION_H
#define LIBSCRIPT_COMPILE_SESSION_H

#include "libscriptdefs.h"

#include "script/diagnosticmessage.h"
#include "script/function.h"
#include "script/lambda.h"
#include "script/script.h"

#include <vector>

namespace script
{

namespace compiler
{

class Compiler;

class CompileSession
{
private:
  Engine * mEngine;

public:
  CompileSession(Engine *e);

  inline Engine* engine() const { return mEngine; }

  struct {
    /// TODO : store a list of generated scripts
    std::vector<ClosureType> lambdas;
    std::vector<Function> functions; /// generated function template instances
    std::vector<Class> classes; /// generated class template instances
    std::shared_ptr<program::Expression> expression;
    std::vector<Script> scripts;
  } generated;

  std::vector<diagnostic::Message> messages;
  bool error;

  void log(const diagnostic::Message & mssg);
  void log(const CompilerException & exception);

  ClosureType newLambda();

  void clear();
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILE_SESSION_H
