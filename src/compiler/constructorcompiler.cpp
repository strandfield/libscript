// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/constructorcompiler.h"

#include "script/compiler/compilererrors.h"
#include "script/compiler/compilesession.h"
#include "script/compiler/expressioncompiler.h"
#include "script/compiler/valueconstructor.h"

#include "script/ast/node.h"

#include "script/program/statements.h"

#include "script/class.h"
#include "script/datamember.h"
#include "script/initialization.h"
#include "script/namelookup.h"
#include "script/overloadresolution.h"
#include "script/typesystem.h"

namespace script
{

namespace compiler
{

ConstructorCompiler::ConstructorCompiler(FunctionCompiler *c)
  : FunctionCompilerExtension(c)
{
  assert(c != nullptr);
}

std::shared_ptr<program::CompoundStatement> ConstructorCompiler::generateHeader()
{
  /// TODO : refactor 
  const Class current_class = currentClass();

  auto ctor_decl = std::static_pointer_cast<ast::ConstructorDecl>(declaration());

  TranslationTarget main_target{ this, ctor_decl };

  auto this_object = ec().implicit_object();

  std::vector<ast::MemberInitialization> initializers = ctor_decl->memberInitializationList;

  std::shared_ptr<program::Statement> parent_ctor_call;
  for (size_t i(0); i < initializers.size(); ++i)
  {
    const auto & minit = initializers.at(i);

    TranslationTarget target{ this, minit.name };

    NameLookup lookup = resolve(minit.name);

    if (lookup.typeResult() == current_class.id()) // delegating constructor
    {
      if (initializers.size() != 1)
        throw CompilationFailure{ CompilerError::InvalidUseOfDelegatedConstructor };

      TranslationTarget nested_target{ this, minit.init };

      if (minit.init->is<ast::ConstructorInitialization>())
      {
        std::vector<std::shared_ptr<program::Expression>> args = ec().generateExpressions(minit.init->as<ast::ConstructorInitialization>().args);
        return program::CompoundStatement::New({ generateDelegateConstructorCall(std::dynamic_pointer_cast<ast::ConstructorInitialization>(minit.init), args) });
      }
      else
      {
        std::vector<std::shared_ptr<program::Expression>> args = ec().generateExpressions(minit.init->as<ast::BraceInitialization>().args);
        return program::CompoundStatement::New({ generateDelegateConstructorCall(std::dynamic_pointer_cast<ast::BraceInitialization>(minit.init), args) });
      }
    }
    else if (!current_class.parent().isNull() && lookup.typeResult() == current_class.parent().id()) // parent constructor call
    {
      TranslationTarget nested_target{ this, minit.init };

      if (minit.init->is<ast::ConstructorInitialization>())
      {
        std::vector<std::shared_ptr<program::Expression>> args = ec().generateExpressions(minit.init->as<ast::ConstructorInitialization>().args);
        parent_ctor_call = generateParentConstructorCall(std::dynamic_pointer_cast<ast::ConstructorInitialization>(minit.init), args);
      }
      else
      {
        std::vector<std::shared_ptr<program::Expression>> args = ec().generateExpressions(minit.init->as<ast::BraceInitialization>().args);
        parent_ctor_call = generateParentConstructorCall(std::dynamic_pointer_cast<ast::BraceInitialization>(minit.init), args);
      }

      // removes m-initializer from list
      std::swap(initializers.back(), initializers.at(i));
      initializers.pop_back();
      break;
    }
  }

  if (parent_ctor_call == nullptr && !current_class.parent().isNull())
  {
    std::vector<std::shared_ptr<program::Expression>> args;
    parent_ctor_call = generateParentConstructorCall(std::shared_ptr<ast::ConstructorInitialization>(), args);
  }

  // Initializating data members
  const auto & data_members = current_class.dataMembers();
  const int data_members_offset = current_class.attributesOffset();
  std::vector<std::shared_ptr<program::Statement>> members_initialization{ data_members.size(), nullptr };
  for (const auto & minit : initializers)
  {
    TranslationTarget target{ this, minit.name };

    NameLookup lookup = resolve(minit.name);

    if (lookup.resultType() != NameLookup::DataMemberName)
      throw CompilationFailure{ CompilerError::NotDataMember, errors::DataMemberName{dstr(minit.name)} };

    if (lookup.dataMemberIndex() - data_members_offset < 0)
      throw CompilationFailure{ CompilerError::InheritedDataMember, errors::DataMemberName{dstr(minit.name)} };

    assert(lookup.dataMemberIndex() - data_members_offset < static_cast<int>(members_initialization.size()));

    const int index = lookup.dataMemberIndex() - data_members_offset;
    if (members_initialization.at(index) != nullptr)
      throw CompilationFailure{ CompilerError::DataMemberAlreadyHasInitializer, errors::DataMemberName{data_members.at(index).name} };

    const auto & dm = data_members.at(index);

    std::shared_ptr<program::Expression> member_value;
    if (minit.init->is<ast::ConstructorInitialization>())
      member_value = ValueConstructor::construct(ec(), dm.type, std::dynamic_pointer_cast<ast::ConstructorInitialization>(minit.init));
    else
      member_value = ValueConstructor::construct(ec(), dm.type, std::dynamic_pointer_cast<ast::BraceInitialization>(minit.init));

    members_initialization[index] = program::PushDataMember::New(member_value);
  }

  for (size_t i(0); i < members_initialization.size(); ++i)
  {
    if (members_initialization[i] != nullptr)
      continue;

    const auto & dm = data_members.at(i);

    std::shared_ptr<program::Expression> default_constructed_value = ValueConstructor::construct(engine(), dm.type, nullptr);
    members_initialization[i] = program::PushDataMember::New(default_constructed_value);
  }

  std::vector<std::shared_ptr<program::Statement>> statements;
  auto init_object = program::InitObjectStatement::New(current_class.id());
  statements.push_back(init_object);
  if (parent_ctor_call)
    statements.push_back(parent_ctor_call);
  statements.insert(statements.end(), members_initialization.begin(), members_initialization.end());
  return program::CompoundStatement::New(std::move(statements));
}

std::shared_ptr<program::CompoundStatement> ConstructorCompiler::generateDefaultConstructor(const Class & cla)
{
  auto this_object = program::StackValue::New(1, Type::ref(cla.id()));

  std::shared_ptr<program::Statement> parent_ctor_call;
  if (!cla.parent().isNull())
  {
    Function parent_default_ctor = cla.parent().defaultConstructor();
    if (parent_default_ctor.isNull())
      throw CompilationFailure{ CompilerError::ParentHasNoDefaultConstructor };
    else if (parent_default_ctor.isDeleted())
      throw CompilationFailure{ CompilerError::ParentHasDeletedDefaultConstructor };

    parent_ctor_call = program::ConstructionStatement::New(this_object->type(), parent_default_ctor, { });
  }

  // Initializating data members
  const auto & data_members = cla.dataMembers();
  std::vector<std::shared_ptr<program::Statement>> members_initialization{ data_members.size(), nullptr };
  for (size_t i(0); i < data_members.size(); ++i)
  {
    const auto & dm = data_members.at(i);

    std::shared_ptr<program::Expression> default_constructed_value = ValueConstructor::construct(cla.engine(), dm.type, std::vector<std::shared_ptr<program::Expression>>{}, Initialization{});
    members_initialization[i] = program::PushDataMember::New(default_constructed_value);
  }

  std::vector<std::shared_ptr<program::Statement>> statements;
  if (parent_ctor_call)
    statements.push_back(parent_ctor_call);
  else
    statements.push_back(program::InitObjectStatement::New(cla.id()));
  statements.insert(statements.end(), members_initialization.begin(), members_initialization.end());
  statements.push_back(program::ReturnStatement::New(this_object));
  return program::CompoundStatement::New(std::move(statements));
}

std::shared_ptr<program::CompoundStatement> ConstructorCompiler::generateCopyConstructor(const Class & cla)
{
  auto this_object = program::StackValue::New(1, Type::ref(cla.id()));
  auto other_object = program::StackValue::New(2, Type::cref(cla.id()));

  std::shared_ptr<program::Statement> parent_ctor_call;
  if (!cla.parent().isNull())
  {
    Function parent_copy_ctor = cla.parent().copyConstructor();
    if (parent_copy_ctor.isNull())
      throw CompilationFailure{ CompilerError::ParentHasNoCopyConstructor };
    else if (parent_copy_ctor.isDeleted())
      throw CompilationFailure{ CompilerError::ParentHasDeletedCopyConstructor };

    parent_ctor_call = program::ConstructionStatement::New(this_object->type(), parent_copy_ctor, { other_object });
  }

  // Initializating data members
  const std::vector<script::DataMember> & data_members = cla.dataMembers();
  const int data_members_offset = cla.attributesOffset();
  std::vector<std::shared_ptr<program::Statement>> members_initialization{ data_members.size(), nullptr };
  for (size_t i(0); i < data_members.size(); ++i)
  {
    const script::DataMember& dm = data_members.at(i);

    const std::shared_ptr<program::Expression> member_access = program::MemberAccess::New(dm.type, other_object, i + data_members_offset);

    const Initialization init = Initialization::compute(dm.type, member_access, cla.engine());

    if (!init.isValid())
      throw CompilationFailure{ CompilerError::DataMemberIsNotCopyable };

    members_initialization[i] = program::PushDataMember::New(ValueConstructor::construct(cla.engine(), dm.type, member_access, init));
  }

  std::vector<std::shared_ptr<program::Statement>> statements;
  if (parent_ctor_call)
    statements.push_back(parent_ctor_call);
  else
    statements.push_back(program::InitObjectStatement::New(cla.id()));
  statements.insert(statements.end(), members_initialization.begin(), members_initialization.end());
  statements.push_back(program::ReturnStatement::New(this_object));
  return program::CompoundStatement::New(std::move(statements));
}

std::shared_ptr<program::CompoundStatement> ConstructorCompiler::generateMoveConstructor(const Class & cla)
{
  const TypeSystem* ts = cla.engine()->typeSystem();

  auto this_object = program::StackValue::New(1, Type::ref(cla.id()));
  auto other_object = program::StackValue::New(2, Type::rref(cla.id()));

  std::shared_ptr<program::Statement> parent_ctor_call;
  if (!cla.parent().isNull())
  {
    Function parent_move_ctor = cla.parent().moveConstructor();
    if (!parent_move_ctor.isNull())
    {
      if (parent_move_ctor.isDeleted())
        throw CompilationFailure{ CompilerError::ParentHasDeletedMoveConstructor };

      parent_ctor_call = program::ConstructionStatement::New(this_object->type(), parent_move_ctor, { other_object });
    }
    else
    {
      Function parent_copy_ctor = cla.parent().copyConstructor();
      if (parent_copy_ctor.isNull())
        throw CompilationFailure{ CompilerError::ParentHasNoCopyConstructor };
      else if (parent_copy_ctor.isDeleted())
        throw CompilationFailure{ CompilerError::ParentHasDeletedCopyConstructor };

      parent_ctor_call = program::ConstructionStatement::New(this_object->type(), parent_copy_ctor, { other_object });
    }
  }

  // Initializating data members
  const auto & data_members = cla.dataMembers();
  const int data_members_offset = cla.attributesOffset();
  std::vector<std::shared_ptr<program::Statement>> members_initialization{ data_members.size(), nullptr };
  for (size_t i(0); i < data_members.size(); ++i)
  {
    const auto & dm = data_members.at(i);

    const std::shared_ptr<program::Expression> member_access = program::MemberAccess::New(dm.type, other_object, i + data_members_offset);
    std::shared_ptr<program::Expression> member_value = nullptr;
    if (dm.type.isReference())
      member_value = member_access;
    else
    {
      if (dm.type.isObjectType())
      {
        Function dm_move_ctor = ts->getClass(dm.type).moveConstructor();
        if (!dm_move_ctor.isNull())
        {
          if (dm_move_ctor.isDeleted())
            throw CompilationFailure{ CompilerError::DataMemberIsNotMovable };

          member_value = program::ConstructorCall::New(dm_move_ctor, { member_access });
        }
        else
        {
          Function dm_copy_ctor = ts->getClass(dm.type).copyConstructor();

          if (dm_copy_ctor.isNull() || dm_copy_ctor.isDeleted())
            throw CompilationFailure{ CompilerError::DataMemberIsNotMovable };

          member_value = program::ConstructorCall::New(dm_copy_ctor, { member_access });
        }
      }
      else
      {
        const Initialization init = Initialization::compute(dm.type, member_access, cla.engine());

        if (!init.isValid())
          throw CompilationFailure{ CompilerError::DataMemberIsNotCopyable };

        member_value = ValueConstructor::construct(cla.engine(), dm.type, member_access, init);
      }
    }

    members_initialization[i] = program::PushDataMember::New(member_value);
  }

  std::vector<std::shared_ptr<program::Statement>> statements;
  if (parent_ctor_call)
    statements.push_back(parent_ctor_call);
  else
    statements.push_back(program::InitObjectStatement::New(cla.id()));
  statements.insert(statements.end(), members_initialization.begin(), members_initialization.end());
  statements.push_back(program::ReturnStatement::New(this_object));
  return program::CompoundStatement::New(std::move(statements));
}

void ConstructorCompiler::checkNarrowingConversions(const std::vector<Initialization> & inits, const std::vector<std::shared_ptr<program::Expression>> & args, const Prototype & proto)
{
  for (size_t i(1); i < inits.size(); ++i)
  {
    if (inits.at(i).isNarrowing())
      throw CompilationFailure{ CompilerError::NarrowingConversionInBraceInitialization, errors::NarrowingConversion{args.at(i - 1)->type(), proto.at(i)} };
  }
}

OverloadResolution::Candidate ConstructorCompiler::getDelegateConstructor(const Class & cla, std::vector<std::shared_ptr<program::Expression>> & args)
{
  const std::vector<Function> & ctors = cla.constructors();
  OverloadResolution::Candidate resol = resolve_overloads(ctors, Type(cla.id()), args);
  if (!resol)
    throw CompilationFailure{ CompilerError::NoDelegatingConstructorFound };
  return resol;
}

std::shared_ptr<program::Statement> ConstructorCompiler::makeDelegateConstructorCall(const OverloadResolution::Candidate& resol, std::vector<std::shared_ptr<program::Expression>> & args)
{
  auto object = program::StackValue::New(1, Type::ref(currentClass().id()));
  Function ctor = resol.function;
  const auto & inits = resol.initializations;
  ValueConstructor::prepare(engine(), object, args, ctor.prototype(), inits);
  return program::ConstructionStatement::New(object->type(), ctor, std::move(args));
}

std::shared_ptr<program::Statement> ConstructorCompiler::generateDelegateConstructorCall(const std::shared_ptr<ast::ConstructorInitialization> & init, std::vector<std::shared_ptr<program::Expression>> & args)
{
  return makeDelegateConstructorCall(getDelegateConstructor(currentClass(), args), args);
}

std::shared_ptr<program::Statement> ConstructorCompiler::generateDelegateConstructorCall(const std::shared_ptr<ast::BraceInitialization> & init, std::vector<std::shared_ptr<program::Expression>> & args)
{
  auto resol = getDelegateConstructor(currentClass(), args);
  checkNarrowingConversions(resol.initializations, args, resol.function.prototype());
  return makeDelegateConstructorCall(resol, args);
}


OverloadResolution::Candidate ConstructorCompiler::getParentConstructor(const Class & cla, std::vector<std::shared_ptr<program::Expression>> & args)
{
  const std::vector<Function> & ctors = cla.parent().constructors();
  OverloadResolution::Candidate resol = resolve_overloads(ctors, Type(cla.id()), args);
  if (!resol)
    throw CompilationFailure{ CompilerError::CouldNotFindValidBaseConstructor };
  return resol;
}

std::shared_ptr<program::Statement> ConstructorCompiler::makeParentConstructorCall(const OverloadResolution::Candidate& resol, std::vector<std::shared_ptr<program::Expression>> & args)
{
  auto object = program::StackValue::New(1, Type::ref(currentClass().id()));
  Function ctor = resol.function;
  const auto & inits = resol.initializations;
  ValueConstructor::prepare(engine(), object, args, ctor.prototype(), inits);
  return program::ConstructionStatement::New(object->type(), ctor, std::move(args));
}

std::shared_ptr<program::Statement> ConstructorCompiler::generateParentConstructorCall(const std::shared_ptr<ast::ConstructorInitialization> & init, std::vector<std::shared_ptr<program::Expression>> & args)
{
  return makeParentConstructorCall(getParentConstructor(currentClass(), args), args);
}

std::shared_ptr<program::Statement> ConstructorCompiler::generateParentConstructorCall(const std::shared_ptr<ast::BraceInitialization> & init, std::vector<std::shared_ptr<program::Expression>> & args)
{
  auto resol = getParentConstructor(currentClass(), args);
  checkNarrowingConversions(resol.initializations, args, resol.function.prototype());
  return makeParentConstructorCall(resol, args);
}

} // namespace compiler

} // namespace script

