// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_COMPILER_EXTENSION_H
#define LIBSCRIPT_FUNCTION_COMPILER_EXTENSION_H

#include "script/compiler/functioncompiler.h"

#include "script/namelookup.h"

namespace script
{

namespace compiler
{

class FunctionCompilerExtension
{
private:
  FunctionCompiler *mCompiler;
public:
  explicit FunctionCompilerExtension(FunctionCompiler *c)
    : mCompiler(c)
  {

  }

  ~FunctionCompilerExtension() = default;

  inline Class currentClass() const { return mCompiler->classScope(); }
  inline FunctionCompiler * compiler() const { return mCompiler; }
  inline const std::shared_ptr<ast::Declaration> & declaration() const { return mCompiler->declaration(); }
  inline Engine * engine() const { return mCompiler->engine(); }
  inline Stack & stack() { return mCompiler->mStack; }

  inline ExpressionCompiler & ec() { return mCompiler->expr_; }

  inline diagnostic::pos_t dpos(const std::shared_ptr<ast::Node> & node) const { return mCompiler->dpos(node); }
  inline std::string dstr(const Type & t) const { return mCompiler->dstr(t); }
  inline static std::string dstr(const std::shared_ptr<ast::Identifier> & id) { return id->getName(); }

  inline NameLookup resolve(const std::shared_ptr<ast::Identifier> & name) const { return mCompiler->resolve(name); }
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_FUNCTION_COMPILER_EXTENSION_H
