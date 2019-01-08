// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_IMPORT_PROCESSOR_H
#define LIBSCRIPT_COMPILER_IMPORT_PROCESSOR_H

#include "script/module.h"
#include "script/scope.h"

#include "script/ast/forwards.h"

namespace script
{

namespace compiler
{

class ImportProcessor
{
public:
  Engine *engine_;

public:
  ImportProcessor(Engine *e);

  inline Engine* engine() const { return engine_; }

  Scope process(const std::shared_ptr<ast::ImportDirective> & decl);
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_IMPORT_PROCESSOR_H
