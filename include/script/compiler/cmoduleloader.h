// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_MODULE_LOADER_H
#define LIBSCRIPT_COMPILER_MODULE_LOADER_H

#include "script/compiler/importprocessor.h"

namespace script
{

namespace compiler
{

class Compiler;

class CModuleLoader : public ModuleLoader
{
public:
  Compiler* compiler_;
public:
  CModuleLoader(Compiler *c);
  ~CModuleLoader() = default;

  Script load(Engine *e, const SourceFile & src) override;
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_MODULE_LOADER_H
