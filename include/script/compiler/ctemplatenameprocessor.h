// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_TEMPLATE_NAME_PROCESSOR_H
#define LIBSCRIPT_COMPILER_TEMPLATE_NAME_PROCESSOR_H

#include "script/templatenameprocessor.h"

namespace script
{

namespace compiler
{

class Compiler;

class LIBSCRIPT_API CTemplateNameProcessor : public TemplateNameProcessor
{
public:
  Compiler* compiler_;
public:
  CTemplateNameProcessor(Compiler *c) : compiler_(c) { }
  ~CTemplateNameProcessor() = default;

  Class instantiate(ClassTemplate & ct, const std::vector<TemplateArgument> & args) override;
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_TEMPLATE_NAME_PROCESSOR_H
