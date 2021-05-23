// Copyright (C) 2018-2021 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/enumbuilder.h"

#include "script/engine.h"
#include "script/enumerator.h"
#include "script/value.h"
#include "script/typesystem.h"

#include "script/functionbuilder.h"

#include "script/interpreter/executioncontext.h"

#include "script/private/class_p.h"
#include "script/private/engine_p.h"
#include "script/private/enum_p.h"
#include "script/private/namespace_p.h"
#include "script/private/operator_p.h"
#include "script/private/typesystem_p.h"
#include "script/private/value_p.h"

namespace script
{

namespace callbacks
{

Value enum_from_int(interpreter::FunctionCall* c)
{
  Enumerator ev{ c->engine()->typeSystem()->getEnum(c->callee().returnType()), c->arg(0).toInt() };
  return Value(new EnumeratorValue(ev));
}

Value enum_copy(interpreter::FunctionCall* c)
{
  return Value(new EnumeratorValue(c->arg(0).toEnumerator()));
}

Value enum_assignment(interpreter::FunctionCall *c)
{
  script::get<Enumerator>(c->arg(0)) = c->arg(1).toEnumerator();
  return c->arg(0);
}

} // namespace callbacks

EnumBuilder& EnumBuilder::operator()(std::string n)
{
  this->name = std::move(n);
  this->is_enum_class = false;
  this->id = 0;
  this->from_int_callback = nullptr;
  this->copy_callback = nullptr;
  this->assignment_callback = nullptr;

  return *this;
}

Enum EnumBuilder::get()
{
  auto impl = std::make_shared<EnumImpl>(0, std::move(name), symbol.engine());
  impl->enclosing_symbol = symbol.impl();
  impl->enumClass = is_enum_class;

  Enum result{ impl };
  
  symbol.engine()->typeSystem()->impl()->register_enum(result, id);

  // E(int)
  {
    DynamicPrototype proto{ Type(result.id()), {Type::cref(Type::Int)} };
    auto ctor = std::make_shared<RegularFunctionImpl>(result.name(), std::move(proto), symbol.engine(), FunctionFlags{});
    ctor->program_ = builders::make_body(this->from_int_callback != nullptr ? this->from_int_callback : callbacks::enum_from_int);
    impl->from_int = Function(ctor);
  }

  // E(const E&)
  {
    DynamicPrototype proto{ Type(result.id()), {Type::cref(result.id())} };
    auto ctor = std::make_shared<RegularFunctionImpl>(result.name(), std::move(proto), symbol.engine(), FunctionFlags{});
    ctor->program_ = builders::make_body(this->copy_callback != nullptr ? this->copy_callback : callbacks::enum_copy);
    impl->copy = Function(ctor);
  }

  // E operator=(const E&)
  {
    BinaryOperatorPrototype proto{ Type::ref(result.id()), Type::ref(result.id()), Type::cref(result.id()) };
    auto op = std::make_shared<BinaryOperatorImpl>(AssignmentOperator, proto, symbol.engine(), FunctionFlags{});
    op->program_ = builders::make_body(this->assignment_callback != nullptr ? this->assignment_callback : callbacks::enum_assignment);
    impl->assignment = Operator{ op };
  }

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
