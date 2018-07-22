// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_CONSTRUCTOR_COMPILER_H
#define LIBSCRIPT_CONSTRUCTOR_COMPILER_H

#include "script/compiler/functioncompilerextension.h"

#include "script/ast/forwards.h"

#include <vector>

namespace script
{

class ConversionSequence;
class OverloadResolution;
class Prototype;

namespace program
{
class CompoundStatement;
class Expression;
class Statement;
} // namespace program

namespace compiler
{

class ConstructorCompiler : public FunctionCompilerExtension
{

public:
  explicit ConstructorCompiler(FunctionCompiler *c);

  std::shared_ptr<program::CompoundStatement> generateHeader();

  std::shared_ptr<program::CompoundStatement> generateDefaultConstructor();
  std::shared_ptr<program::CompoundStatement> generateCopyConstructor();
  std::shared_ptr<program::CompoundStatement> generateMoveConstructor();

protected:
  void checkNarrowingConversions(const std::vector<ConversionSequence> & convs, const std::vector<std::shared_ptr<program::Expression>> & args, const Prototype & proto);

  OverloadResolution getDelegateConstructor(std::vector<std::shared_ptr<program::Expression>> & args);
  std::shared_ptr<program::Statement> makeDelegateConstructorCall(const OverloadResolution & resol, std::vector<std::shared_ptr<program::Expression>> & args);
  std::shared_ptr<program::Statement> generateDelegateConstructorCall(const std::shared_ptr<ast::ConstructorInitialization> & init, std::vector<std::shared_ptr<program::Expression>> & args);
  std::shared_ptr<program::Statement> generateDelegateConstructorCall(const std::shared_ptr<ast::BraceInitialization> & init, std::vector<std::shared_ptr<program::Expression>> & args);
  
  OverloadResolution getParentConstructor(std::vector<std::shared_ptr<program::Expression>> & args);
  std::shared_ptr<program::Statement> makeParentConstructorCall(const OverloadResolution & resol, std::vector<std::shared_ptr<program::Expression>> & args);
  std::shared_ptr<program::Statement> generateParentConstructorCall(const std::shared_ptr<ast::ConstructorInitialization> & init, std::vector<std::shared_ptr<program::Expression>> & args);
  std::shared_ptr<program::Statement> generateParentConstructorCall(const std::shared_ptr<ast::BraceInitialization> & init, std::vector<std::shared_ptr<program::Expression>> & args);
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_CONSTRUCTOR_COMPILER_H
