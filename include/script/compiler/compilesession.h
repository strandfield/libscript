// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILE_SESSION_H
#define LIBSCRIPT_COMPILE_SESSION_H

#include "libscriptdefs.h"

#include "script/compiler/cfunctiontemplateprocessor.h"
#include "script/compiler/cmoduleloader.h"
#include "script/compiler/ctemplatenameprocessor.h"
#include "script/compiler/logger.h"

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
class CompileSession;

class SessionLogger : public Logger
{
public:
  CompileSession *session_;

public:
  SessionLogger(CompileSession *s)
    : session_(s)
  {

  }

  ~SessionLogger() = default;

  void log(const diagnostic::Message & mssg) override;
  void log(const CompilerException & exception) override;
};

/// TODO: should we create a queue of Function to compile, and remove the 
// responsability away from ScriptCompiler
class CompileSession
{
public:
  enum class State {
    ProcessingDeclarations,
    CompilingFunctions,
    Finished,
  };

private:
  Compiler* mCompiler;
  State mState;

public:
  CompileSession(Compiler *c);
  CompileSession(Compiler *c, const Script & s);


  inline Compiler* compiler() const { return mCompiler; }
  Engine* engine() const;
  inline State state() const { return mState; }
  inline void setState(State t) { mState = t; }

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

  SessionLogger mLogger;
  CTemplateNameProcessor mTNP;
  CFunctionTemplateProcessor mFTP;
  CModuleLoader mLoader;

  void log(const diagnostic::Message & mssg);
  void log(const CompilerException & exception);

  void clear();
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILE_SESSION_H
