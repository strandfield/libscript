// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_COMPONENT_H
#define LIBSCRIPT_COMPILER_COMPONENT_H

#include "libscriptdefs.h"

#include <memory>

namespace script
{

class Engine;

namespace diagnostic
{
class DiagnosticMessage;
} // namespace diagnostic

namespace compiler
{

class Compiler;
class CompileSession;

class LIBSCRIPT_API Component
{
private:
  Compiler* m_compiler;

public:
  explicit Component(Compiler* c);
  ~Component();

  Engine* engine() const;
  Compiler* compiler() const;
  const std::shared_ptr<CompileSession>& session() const;

protected:
  void log(const diagnostic::DiagnosticMessage& mssg);
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_COMPONENT_H
