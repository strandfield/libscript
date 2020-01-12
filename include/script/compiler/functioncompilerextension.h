// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_COMPILER_EXTENSION_H
#define LIBSCRIPT_FUNCTION_COMPILER_EXTENSION_H

#include "script/compiler/component.h"

#include <memory>
#include <string>

namespace script
{

class Class;
class Engine;
class NameLookup;

namespace ast
{
class Declaration;
class Identifier;
} // namespace ast

namespace compiler
{

class ExpressionCompiler;
class FunctionCompiler;
class Stack;

class FunctionCompilerExtension : public Component
{
private:
  FunctionCompiler *m_function_compiler;

public:
  explicit FunctionCompilerExtension(FunctionCompiler* c);

  ~FunctionCompilerExtension() = default;

  Class currentClass() const;
  FunctionCompiler * compiler() const;
  const std::shared_ptr<ast::Declaration> & declaration() const;
  Engine * engine() const;
  Stack & stack();

  ExpressionCompiler & ec();

  static std::string dstr(const std::shared_ptr<ast::Identifier> & id);

  NameLookup resolve(const std::shared_ptr<ast::Identifier> & name);
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_FUNCTION_COMPILER_EXTENSION_H
