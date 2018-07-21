// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/assignmentcompiler.h"
#include "script/compiler/compilererrors.h"

#include "script/ast/node.h"

#include "script/program/statements.h"

#include "script/datamember.h"
#include "script/namelookup.h"
#include "script/overloadresolution.h"

namespace script
{

namespace compiler
{

AssignmentCompiler::AssignmentCompiler(FunctionCompiler *c)
  : FunctionCompilerExtension(c)
{
  assert(c != nullptr);
}

std::shared_ptr<program::CompoundStatement> AssignmentCompiler::generateAssignmentOperator()
{
  const Class current_class = currentClass();

  auto this_object = ec().implicit_object();
  auto other_object = program::StackValue::New(2, stack()[2].type);

  std::shared_ptr<program::Statement> parent_assign_call;
  if (!current_class.parent().isNull())
  {
    Operator parent_assign = findAssignmentOperator(current_class.parent().id());
    if (parent_assign.isNull())
      throw ParentHasNoAssignmentOperator{};
    else if (parent_assign.isDeleted())
      throw ParentHasDeletedAssignmentOperator{};

    parent_assign_call = program::ExpressionStatement::New(program::FunctionCall::New(parent_assign, { this_object, other_object }));
  }

  // Assigns data members
  const auto & data_members = current_class.dataMembers();
  const int data_members_offset = current_class.attributesOffset();
  std::vector<std::shared_ptr<program::Statement>> members_assign{ data_members.size(), nullptr };
  for (size_t i(0); i < data_members.size(); ++i)
  {
    const auto & dm = data_members.at(i);
    if (dm.type.isReference())
      throw DataMemberIsReferenceAndCannotBeAssigned{};
    if (dm.type.isConst())
      throw NotImplementedError{ "Data member is const and cannot be assigned" };

    Operator dm_assign = findAssignmentOperator(dm.type);
    if (dm_assign.isNull())
      throw DataMemberHasNoAssignmentOperator{};
    else if (dm_assign.isDeleted())
      throw DataMemberHasDeletedAssignmentOperator{};

    auto fetch_this_member = program::MemberAccess::New(dm.type, this_object, i + data_members_offset);
    auto fetch_other_member = program::MemberAccess::New(dm.type, other_object, i + data_members_offset);

    auto assign = program::FunctionCall::New(dm_assign, { fetch_this_member, fetch_other_member });
    members_assign[i] = program::ExpressionStatement::New(assign);
  }

  std::vector<std::shared_ptr<program::Statement>> statements;
  if (parent_assign_call)
    statements.push_back(parent_assign_call);
  statements.insert(statements.end(), members_assign.begin(), members_assign.end());
  statements.insert(statements.end(), program::ReturnStatement::New(this_object));
  return program::CompoundStatement::New(std::move(statements));
}

bool AssignmentCompiler::isAssignmentOperator(const Operator & op, const Type & t)
{
  if (op.operatorId() != AssignmentOperator)
    return false;

  if (op.returnType() != Type::ref(t.baseType()))
    return false;
  if (op.firstOperand() != Type::ref(t.baseType()))
    return false;
  if (op.secondOperand() != Type::cref(t.baseType()))
    return false;

  return true;
}

Operator AssignmentCompiler::findAssignmentOperator(const Type & t)
{
  if (t.isFundamentalType())
  {
    const auto & ops = engine()->rootNamespace().operators();
    for (const auto & o : ops)
    {
      if (isAssignmentOperator(o, t))
        return o;
    }

  }
  else if (t.isEnumType())
  {
    return engine()->getEnum(t).getAssignmentOperator();
  }
  else if (t.isObjectType())
  {
    const auto & ops = engine()->getClass(t).operators();
    for (const auto & o : ops)
    {
      if (isAssignmentOperator(o, t))
        return o;
    }
  }

  return Operator{};
}


} // namespace compiler

} // namespace script
