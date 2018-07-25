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

/// TODO: should we create a queue of Function to compile, and remove the 
// responsability away from ScriptCompiler
class CompileSession
{
private:
  Engine* mEngine;
  bool mIsActive;

public:
  CompileSession(Engine *e);
  CompileSession(const Script & s);

  inline Engine* engine() const { return mEngine; }

  struct {
    /// TODO : ideally we should store a list of generated lambdas and function types
    std::vector<Function> functions; /// generated function template instances
    std::vector<Class> classes; /// generated class template instances
    std::shared_ptr<program::Expression> expression;
    std::vector<Script> scripts;
  } generated;

  std::vector<diagnostic::Message> messages;
  bool error;
  Script script;

  void log(const diagnostic::Message & mssg);
  void log(const CompilerException & exception);

  void clear();

  inline bool is_active() const { return mIsActive; }
  inline void set_active(bool act) { mIsActive = act; }
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILE_SESSION_H
