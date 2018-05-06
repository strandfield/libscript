// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_IMPORT_PROCESSOR_H
#define LIBSCRIPT_COMPILER_IMPORT_PROCESSOR_H

#include "script/scope.h"

#include "script/compiler/expressioncompiler.h"

#include "script/ast/forwards.h"
#include "script/program/expression.h"

namespace script
{

namespace compiler
{

template<typename ModuleLoader>
class ImportProcessor
{
public:
  ModuleLoader loader_;
  Scope result_;

public:

  inline Engine* engine() const { return loader_.engine(); }

  Scope process(const std::shared_ptr<ast::ImportDirective> & decl)
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

  Scope load_script_module(const std::shared_ptr<ast::ImportDirective> & decl)
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

  Scope load(const support::filesystem::path & p)
  {
    if (p.extension() != engine()->scriptExtension())
      return Scope{};

    Script s;
    if (is_loaded(p, s))
    {
      /// TODO : also import exported content
      return Scope{ s.rootNamespace() };
    }

    s = loader_.load(SourceFile{ p.string() });

    return Scope{ s.rootNamespace() };
  }

  void load_recursively(const support::filesystem::path & dir)
  {
    for (auto& p : support::filesystem::directory_iterator(dir))
    {
      if (support::filesystem::is_directory(p))
        load_recursively(p);
      else
        add_import(load(p));
    }
  }

  bool is_loaded(const support::filesystem::path & p, Script & result)
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

private:
  void add_import(const Scope & scp)
  {
    if (result_.isNull())
      result_ = scp;
    else
      result_.merge(scp);
  }

};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_IMPORT_PROCESSOR_H
