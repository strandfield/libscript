// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/variableaccessor.h"

#include "script/compiler/diagnostichelper.h"
#include "script/compiler/compilererrors.h"
#include "script/compiler/expressioncompiler.h"
#include "script/compiler/stack.h"

#include "script/program/expression.h"

#include "script/class.h"
#include "script/datamember.h"
#include "script/engine.h"
#include "script/lambda.h"
#include "script/script.h"
#include "script/typesystem.h"

#include "script/private/script_p.h"

namespace script
{

namespace compiler
{

VariableAccessor::VariableAccessor(Stack* s)
  : stack_(s)
{

}

void VariableAccessor::setStack(Stack* s)
{
  stack_ = s;
}

const Stack& VariableAccessor::stack() const
{
  return *stack_;
}

std::shared_ptr<program::Expression> VariableAccessor::accessDataMember(ExpressionCompiler & ec, int offset)
{
  if (ec.caller().memberOf().isClosure())
  {
    ClosureType ct = ec.caller().memberOf().toClosure();
    auto lambda = program::StackValue::New(1, Type::ref(ct.id()));
    auto this_object = program::CaptureAccess::New(Type::ref(ct.captures().at(0).type), lambda, 0);
    return generateMemberAccess(ec, this_object, offset);
  }
  else
  {
    auto object = ec.implicit_object();
    return generateMemberAccess(ec, object, offset);
  }
}

std::shared_ptr<program::Expression> VariableAccessor::accessGlobal(ExpressionCompiler & ec, int offset)
{
  Script s = ec.caller().script();
  auto simpl = s.impl();
  const Type & gtype = simpl->global_types[offset];

  return program::FetchGlobal::New(s.id(), offset, gtype);
}

std::shared_ptr<program::Expression> VariableAccessor::accessLocal(ExpressionCompiler & ec, int offset)
{
  const Type t = stack()[offset].type;
  return program::StackValue::New(offset, t);
}

std::shared_ptr<program::Expression> VariableAccessor::accessCapture(ExpressionCompiler & ec, int offset)
{
  ClosureType ct = ec.caller().memberOf().toClosure();
  auto lambda = program::StackValue::New(1, Type::ref(ct.id()));
  const auto capture = ct.captures().at(offset);
  auto capaccess = program::CaptureAccess::New(capture.type, lambda, offset);
  captures_.push_back(capaccess);
  return capaccess;
}

std::shared_ptr<program::Expression> VariableAccessor::generateMemberAccess(ExpressionCompiler & ec, const std::shared_ptr<program::Expression> & object, const int offset)
{
  Class cla = ec.engine()->typeSystem()->getClass(object->type());
  int relative_index = offset;
  while (relative_index - int(cla.dataMembers().size()) >= 0)
  {
    relative_index = relative_index - static_cast<int>(cla.dataMembers().size());
    cla = cla.parent();
  }

  const auto & dm = cla.dataMembers().at(relative_index);

  if (!Accessibility::check(ec.caller(), cla, dm.accessibility()))
    throw CompilationFailure{ CompilerError::InaccessibleMember, errors::InaccessibleMember{dm.name, dm.accessibility()} };

  const Type access_type = object->type().isConst() ? Type::cref(dm.type) : Type::ref(dm.type);
  return program::MemberAccess::New(access_type, object, offset);
}

} // namespace compiler

} // namespace script

