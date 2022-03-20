// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_CONSTRUCTOR_COMPILER_H
#define LIBSCRIPT_CONSTRUCTOR_COMPILER_H

#include "script/compiler/functioncompilerextension.h"

#include "script/ast/forwards.h"

#include "script/overloadresolution.h"

#include <vector>

namespace script
{

class Initialization;
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

  // @TODO: with the refactor of FunctionBuilder, maybe these no longer need to be static
  static std::shared_ptr<program::CompoundStatement> generateDefaultConstructor(const Class & cla);
  static std::shared_ptr<program::CompoundStatement> generateCopyConstructor(const Class & cla);
  static std::shared_ptr<program::CompoundStatement> generateMoveConstructor(const Class & cla);

protected:
  static void checkNarrowingConversions(const std::vector<Initialization> & inits, const std::vector<std::shared_ptr<program::Expression>> & args, const Prototype & proto);

  static OverloadResolution::Candidate getDelegateConstructor(const Class & cla, std::vector<std::shared_ptr<program::Expression>> & args);
  std::shared_ptr<program::Statement> makeDelegateConstructorCall(const OverloadResolution::Candidate& resol, std::vector<std::shared_ptr<program::Expression>> & args);
  std::shared_ptr<program::Statement> generateDelegateConstructorCall(const std::shared_ptr<ast::ConstructorInitialization> & init, std::vector<std::shared_ptr<program::Expression>> & args);
  std::shared_ptr<program::Statement> generateDelegateConstructorCall(const std::shared_ptr<ast::BraceInitialization> & init, std::vector<std::shared_ptr<program::Expression>> & args);
  
  static OverloadResolution::Candidate getParentConstructor(const Class & cla, std::vector<std::shared_ptr<program::Expression>> & args);
  std::shared_ptr<program::Statement> makeParentConstructorCall(const OverloadResolution::Candidate& resol, std::vector<std::shared_ptr<program::Expression>> & args);
  std::shared_ptr<program::Statement> generateParentConstructorCall(const std::shared_ptr<ast::ConstructorInitialization> & init, std::vector<std::shared_ptr<program::Expression>> & args);
  std::shared_ptr<program::Statement> generateParentConstructorCall(const std::shared_ptr<ast::BraceInitialization> & init, std::vector<std::shared_ptr<program::Expression>> & args);
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_CONSTRUCTOR_COMPILER_H
