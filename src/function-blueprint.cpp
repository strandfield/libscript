// Copyright (C) 2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/function-blueprint.h"

#include "script/class.h"
#include "script/namespace.h"

namespace script
{

/*!
 * \class FunctionBlueprint
 */

FunctionBlueprint::FunctionBlueprint(Symbol s, SymbolKind k, std::string name)
  : parent_(s),
    name_(k, name)
{
  prototype_.setReturnType(Type::Void);

  if (s.isClass())
    prototype_.push(Type::ref(s.toClass().id()).withFlag(Type::ThisFlag));
}

FunctionBlueprint::FunctionBlueprint(Symbol s, SymbolKind k, Type t)
  : parent_(s),
    name_(k, t)
{
  if (k == SymbolKind::Cast)
  {
    prototype_.setReturnType(t);
  }
  else if (k == SymbolKind::Constructor || k == SymbolKind::Destructor)
  {
    prototype_.setReturnType(Type::Void);
  }

  // @TODO: assert(s.isClass()) ?
  if (s.isClass())
    prototype_.push(Type::ref(s.toClass().id()).withFlag(Type::ThisFlag));
}

FunctionBlueprint::FunctionBlueprint(Symbol s, SymbolKind k, OperatorName n)
  : parent_(s),
    name_(k, n)
{
  prototype_.setReturnType(Type::Void);

  if(s.isClass())
    prototype_.push(Type::ref(s.toClass().id()).withFlag(Type::ThisFlag));
}

FunctionBlueprint::FunctionBlueprint(Symbol s)
  : parent_(s)
{

}

FunctionBlueprint FunctionBlueprint::Fun(Class c, std::string name)
{
  return FunctionBlueprint(Symbol(c), SymbolKind::Function, std::move(name));
}

FunctionBlueprint FunctionBlueprint::Fun(Namespace ns, std::string name)
{
  return FunctionBlueprint(Symbol(ns), SymbolKind::Function, std::move(name));
}

FunctionBlueprint FunctionBlueprint::Constructor(Class c)
{
  return FunctionBlueprint(Symbol(c), SymbolKind::Constructor, Type(c.id()));
}

FunctionBlueprint FunctionBlueprint::Destructor(Class c)
{
  return FunctionBlueprint(Symbol(c), SymbolKind::Destructor, Type(c.id()));
}

FunctionBlueprint FunctionBlueprint::Op(Class c, OperatorName op)
{
  return FunctionBlueprint(Symbol(c), SymbolKind::Operator, op);
}

FunctionBlueprint FunctionBlueprint::Op(Namespace ns, OperatorName op)
{
  return FunctionBlueprint(Symbol(ns), SymbolKind::Operator, op);
}

FunctionBlueprint FunctionBlueprint::LiteralOp(Namespace ns, std::string suffix)
{
  return FunctionBlueprint(Symbol(ns), SymbolKind::LiteralOperator, std::move(suffix));
}

FunctionBlueprint FunctionBlueprint::Cast(Class c)
{
  return FunctionBlueprint(Symbol(c), SymbolKind::Cast, Type::Null);
}

/*!
 * \endclass
 */

} // namespace script
