// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/enumbuilder.h"

#include "script/engine.h"
#include "script/enumerator.h"
#include "script/value.h"

#include "script/interpreter/executioncontext.h"

#include "script/private/class_p.h"
#include "script/private/engine_p.h"
#include "script/private/enum_p.h"
#include "script/private/namespace_p.h"
#include "script/private/operator_p.h"
#include "script/private/value_p.h"

namespace script
{

namespace callbacks
{

Value enum_assignment(interpreter::FunctionCall *c)
{
  c->arg(0).impl()->set_enumerator(c->arg(1).toEnumerator());
  return c->arg(0);
}

} // namespace callbacks

Enum EnumBuilder::get()
{
  auto impl = std::make_shared<EnumImpl>(0, std::move(name), symbol.engine());
  impl->enclosing_symbol = symbol.impl();
  impl->enumClass = is_enum_class;

  Enum result{ impl };
  
  symbol.engine()->implementation()->register_enum(result, id);

  BinaryOperatorPrototype proto{ Type::ref(result.id()), Type::ref(result.id()), Type::cref(result.id()) };
  auto op = std::make_shared<BinaryOperatorImpl>(AssignmentOperator, proto, symbol.engine());
  op->implementation.callback = callbacks::enum_assignment;
  impl->assignment = Operator{ op };

  if (symbol.isClass())
    std::static_pointer_cast<ClassImpl>(symbol.impl())->enums.push_back(result);
  else
    std::static_pointer_cast<NamespaceImpl>(symbol.impl())->enums.push_back(result);

  return result;
}

void EnumBuilder::create()
{
  get();
}

} // namespace script
