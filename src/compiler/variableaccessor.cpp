// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/variableaccessor.h"

#include "script/compiler/diagnostichelper.h"
#include "script/compiler/expressioncompiler.h"

#include "script/program/expression.h"

#include "script/class.h"
#include "script/datamember.h"
#include "script/script.h"

#include "script/private/script_p.h"

namespace script
{

namespace compiler
{

std::shared_ptr<program::Expression> VariableAccessor::accessDataMember(ExpressionCompiler & ec, int offset, const diagnostic::pos_t dpos)
{
  auto object = ec.implicit_object();
  return generateMemberAccess(ec, object, offset, dpos);
}

std::shared_ptr<program::Expression> VariableAccessor::accessGlobal(ExpressionCompiler & ec, int offset, const diagnostic::pos_t dpos)
{
  Script s = ec.caller().script();
  auto simpl = s.impl();
  const Type & gtype = simpl->global_types[offset];

  return program::FetchGlobal::New(s.id(), offset, gtype);
}

std::shared_ptr<program::Expression> VariableAccessor::accessLocal(ExpressionCompiler & ec, int offset, const diagnostic::pos_t dpos)
{
  /// TODO: should we throw this type of exception ?
  throw std::runtime_error{ "Default VariableAccessor does not support accessing local variables" };
}

std::shared_ptr<program::Expression> VariableAccessor::accessCapture(ExpressionCompiler & ec, int offset, const diagnostic::pos_t dpos)
{
  /// TODO: should we throw this type of exception ?
  throw std::runtime_error{ "Default VariableAccessor does not support accessing captures" };
}

std::shared_ptr<program::Expression> VariableAccessor::generateMemberAccess(ExpressionCompiler & ec, const std::shared_ptr<program::Expression> & object, const int offset, const diagnostic::pos_t dpos)
{
  Class cla = ec.engine()->getClass(object->type());
  int relative_index = offset;
  while (relative_index - int(cla.dataMembers().size()) >= 0)
  {
    relative_index = relative_index - cla.dataMembers().size();
    cla = cla.parent();
  }

  const auto & dm = cla.dataMembers().at(relative_index);

  if (!Accessibility::check(ec.caller(), cla, dm.accessibility()))
    throw InaccessibleMember{ dpos, dm.name, dm.accessibility() };

  const Type access_type = object->type().isConst() ? Type::cref(dm.type) : Type::ref(dm.type);
  return program::MemberAccess::New(access_type, object, offset);
}

} // namespace compiler

} // namespace script

