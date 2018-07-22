// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_COMPONENT_H
#define LIBSCRIPT_COMPILER_COMPONENT_H

#include "libscriptdefs.h"

#include "script/diagnosticmessage.h"

namespace script
{

class Engine;

namespace compiler
{

class Compiler;
class CompilerException;
class CompileSession;

class CompilerComponent
{
private:
  Compiler* mCompiler;

public:
  CompilerComponent(Compiler *c);

  Engine* engine() const;
  inline Compiler* compiler() const { return mCompiler; }
  std::shared_ptr<CompileSession> session() const;

protected:
  void log(const diagnostic::Message & mssg);
  void log(const CompilerException & exception);
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_COMPONENT_H
