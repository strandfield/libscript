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

  inline diagnostic::pos_t dpos(const std::shared_ptr<ast::Node> & node) const { return mCompiler->dpos(node); }
  inline std::string dstr(const Type & t) const { return mCompiler->dstr(t); }
  inline static std::string dstr(const std::shared_ptr<ast::Identifier> & id) { return AbstractExpressionCompiler::dstr(id); }

  inline NameLookup resolve(const std::shared_ptr<ast::Identifier> & name) const { return mCompiler->resolve(name); }

  inline std::shared_ptr<program::Expression> constructValue(const Type & t, std::nullptr_t, diagnostic::pos_t dp) { return mCompiler->constructValue(t, nullptr, dp); }
  inline std::shared_ptr<program::Expression> constructValue(const Type & t, std::vector<std::shared_ptr<program::Expression>> && args, diagnostic::pos_t dp) { return mCompiler->constructValue(t, std::move(args), dp); }
  inline std::shared_ptr<program::Expression> constructValue(const Type & t, const std::shared_ptr<ast::ConstructorInitialization> & init) { return mCompiler->constructValue(t, init); }
  inline std::shared_ptr<program::Expression> constructValue(const Type & t, const std::shared_ptr<ast::BraceInitialization> & init) { return mCompiler->constructValue(t, init); }

  inline std::vector<std::shared_ptr<program::Expression>> generateExpressions(const std::vector<std::shared_ptr<ast::Expression>> & e) { return mCompiler->generateExpressions(e); }

  inline std::shared_ptr<program::Expression> prepareFunctionArgument(const std::shared_ptr<program::Expression> & arg, const Type & type, const ConversionSequence & conv) { return mCompiler->prepareFunctionArgument(arg, type, conv); }
  inline void prepareFunctionArguments(std::vector<std::shared_ptr<program::Expression>> & args, const Prototype & proto, const std::vector<ConversionSequence> & convs) { mCompiler->prepareFunctionArguments(args, proto, convs); }

  inline std::shared_ptr<program::Expression> generateThisAccess() const { return mCompiler->generateThisAccess(); }
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_FUNCTION_COMPILER_EXTENSION_H
