// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_FUNCTION_TEMPLATE_PROCESSOR_H
#define LIBSCRIPT_COMPILER_FUNCTION_TEMPLATE_PROCESSOR_H

#include "script/functiontemplateprocessor.h"

namespace script
{

namespace compiler
{

class Compiler;

class LIBSCRIPT_API CFunctionTemplateProcessor : public FunctionTemplateProcessor
{
public:
  Compiler* compiler_;
public:
  CFunctionTemplateProcessor(Compiler *c) : compiler_(c) { }
  ~CFunctionTemplateProcessor() = default;

  void instantiate(Function & f) override;
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_FUNCTION_TEMPLATE_PROCESSOR_H
