// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/logger.h"

#include "script/compiler/compilererrors.h"

namespace script
{

namespace compiler
{

Logger::~Logger()
{

}

void Logger::log(const diagnostic::DiagnosticMessage & mssg)
{
  (void)mssg;
}

void Logger::log(const CompilerException & exception)
{
  (void)exception;
}

} // namespace compiler

} // namespace script

