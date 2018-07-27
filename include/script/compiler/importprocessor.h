// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_IMPORT_PROCESSOR_H
#define LIBSCRIPT_COMPILER_IMPORT_PROCESSOR_H

#include "script/module.h"
#include "script/scope.h"

#include "script/support/filesystem.h"

#include "script/ast/forwards.h"

namespace script
{

class Engine;
class SourceFile;
class Script;

namespace compiler
{

class ModuleLoader
{
public:
  virtual ~ModuleLoader() = default;

  virtual Script load(Engine *e, const SourceFile & src);
};

class ImportProcessor
{
public:
  Engine *engine_;
  ModuleLoader default_loader_;
  ModuleLoader *loader_;
  Scope result_;

public:
  ImportProcessor(Engine *e);

  inline Engine* engine() const { return engine_; }

  void set_loader(ModuleLoader & l) { loader_ = &l; }
  ModuleLoader & loader() { return *loader_; }

  Scope process(const std::shared_ptr<ast::ImportDirective> & decl);

  Scope load_script_module(const std::shared_ptr<ast::ImportDirective> & decl);

  Scope load(const support::filesystem::path & p);

  void load_recursively(const support::filesystem::path & dir);

  bool is_loaded(const support::filesystem::path & p, Script & result);

private:
  void add_import(const Scope & scp);

};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_IMPORT_PROCESSOR_H
