// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILE_SESSION_H
#define LIBSCRIPT_COMPILE_SESSION_H

#include "libscriptdefs.h"

#include "script/compiler/logger.h"

#include "script/diagnosticmessage.h"
#include "script/function.h"
#include "script/lambda.h"
#include "script/script.h"

#include <vector>

namespace script
{

namespace ast
{
class Node;
} // namespace ast

namespace compiler
{

class Component;
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

  void log(const diagnostic::DiagnosticMessage & mssg) override;
};

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

  std::vector<diagnostic::DiagnosticMessage> messages;
  bool error;
  Script script;

  std::shared_ptr<ast::Node> current_node;
  parser::Token current_token;

  SessionLogger mLogger;

  void log(const diagnostic::DiagnosticMessage & mssg);
  void log(const CompilationFailure& ex);

  void clear();
};

class TranslationTarget
{
private:
  const Component* m_component;
  std::shared_ptr<ast::Node> m_current_node;
  parser::Token m_current_token;

public:
  TranslationTarget(const Component* c, std::shared_ptr<ast::Node> node);
  TranslationTarget(const Component* c, parser::Token tok);
  ~TranslationTarget();
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILE_SESSION_H
