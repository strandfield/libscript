// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_IMPORT_PROCESSOR_H
#define LIBSCRIPT_COMPILER_IMPORT_PROCESSOR_H

#include "script/module.h"
#include "script/scope.h"

#include "script/ast/forwards.h"

#include "script/compiler/component.h"

namespace script
{

namespace compiler
{

class ImportProcessor : public Component
{
public:
  explicit ImportProcessor(Compiler *c);

  Scope process(const std::shared_ptr<ast::ImportDirective> & decl);

protected:
  void load_module(Module& m);
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_IMPORT_PROCESSOR_H
