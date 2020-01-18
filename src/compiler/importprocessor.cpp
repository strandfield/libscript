// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/importprocessor.h"

#include "script/compiler/compilererrors.h"
#include "script/compiler/diagnostichelper.h"

#include "script/ast/node.h"

#include "script/engine.h"
#include "script/module.h"

namespace script
{

namespace compiler
{

ImportProcessor::ImportProcessor(Engine *e)
  : engine_(e)
{

}

Scope ImportProcessor::process(const std::shared_ptr<ast::ImportDirective> & decl)
{
  Module m = engine()->getModule(decl->at(0));

  if (m.isNull())
    throw CompilationFailure{ CompilerError::UnknownModuleName, errors::InvalidName{decl->at(0)} };

  for (size_t i(1); i < decl->size(); ++i)
  {
    Module child = m.getSubModule(decl->at(i));
    if (child.isNull())
      throw CompilationFailure{ CompilerError::UnknownSubModuleName, errors::InvalidName{decl->at(i)} };

    m = child;
  }

  try
  {
    m.load();
  }
  catch (ModuleLoadingError& ex)
  {
    throw CompilationFailure{ CompilerError::ModuleImportationFailed, errors::ModuleImportationFailed{ex.message} };
  }

  return m.scope();
}

} // namespace compiler

} // namespace script

