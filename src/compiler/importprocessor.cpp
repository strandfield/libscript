// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/importprocessor.h"

#include "script/compiler/compilererrors.h"
#include "script/compiler/diagnostichelper.h"

#include "script/ast/node.h"

#include "script/engine.h"
#include "script/module.h"
#include "script/sourcefile.h"

namespace script
{

namespace compiler
{

Script ModuleLoader::load(Engine *e, const SourceFile & src)
{
  Script s = e->newScript(src);
  bool success = e->compile(s); /// TODO: bad, will create another compiler and another session
  if (!success)
  {
    std::string mssg;
    for (const auto & m : s.messages())
      mssg += m.to_string() + "\n";
    throw ModuleImportationError{ src.filepath(), mssg };
  }

  /// TODO: ensure the script is run later
  // s.run();

  return s;
}

ImportProcessor::ImportProcessor(Engine *e)
  : engine_(e)
{
  loader_ = &default_loader_;
}


Scope ImportProcessor::process(const std::shared_ptr<ast::ImportDirective> & decl)
{
  result_ = Scope{};

  Module m = engine()->getModule(decl->at(0));
  if (m.isNull())
  {
    return load_script_module(decl);
  }

  for (size_t i(1); i < decl->size(); ++i)
  {
    Module child = m.getSubModule(decl->at(i));
    if (child.isNull())
      throw UnknownSubModuleName{ dpos(decl), decl->at(i), m.name() };

    m = child;
  }

  m.load();

  return m.scope();
}

Scope ImportProcessor::load_script_module(const std::shared_ptr<ast::ImportDirective> & decl)
{
  auto path = engine()->searchDirectory();

  for (size_t i(0); i < decl->size(); ++i)
    path /= decl->at(i);

  if (support::filesystem::is_directory(path))
  {
    load_recursively(path);
    return result_;
  }
  else
  {
    path += engine()->scriptExtension();

    if (!support::filesystem::exists(path))
      throw UnknownModuleName{ dpos(decl), decl->full_name() };

    return load(path);
  }
}

Scope ImportProcessor::load(const support::filesystem::path & p)
{
  if (p.extension() != engine()->scriptExtension())
    return Scope{};

  Script s;
  if (is_loaded(p, s))
  {
    /// TODO : also import exported content
    return Scope{ s.rootNamespace() };
  }

  s = loader_->load(engine(), SourceFile{ p.string() });

  return Scope{ s.rootNamespace() };
}

void ImportProcessor::load_recursively(const support::filesystem::path & dir)
{
  for (auto& p : support::filesystem::directory_iterator(dir))
  {
    if (support::filesystem::is_directory(p))
      load_recursively(p);
    else
      add_import(load(p));
  }
}

bool ImportProcessor::is_loaded(const support::filesystem::path & p, Script & result)
{
  for (const auto & s : engine()->scripts())
  {
    if (s.path() == p)
    {
      result = s;
      return true;
    }
  }

  return false;
}

void ImportProcessor::add_import(const Scope & scp)
{
  if (result_.isNull())
    result_ = scp;
  else
    result_.merge(scp);
}

} // namespace compiler

} // namespace script

