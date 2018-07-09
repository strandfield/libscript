// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/symbol.h"

#include "script/class.h"
#include "script/functionbuilder.h"
#include "script/namespace.h"
#include "script/private/class_p.h"
#include "script/private/namespace_p.h"
#include "script/private/script_p.h"
#include "script/script.h"

namespace script
{

Script SymbolImpl::getScript(const std::shared_ptr<SymbolImpl> & sym)
{
  if (dynamic_cast<NamespaceImpl*>(sym.get()) != nullptr)
    return Namespace{ std::dynamic_pointer_cast<NamespaceImpl>(sym) }.script();
  else if (dynamic_cast<ClassImpl*>(sym.get()) != nullptr)
    return Class{ std::dynamic_pointer_cast<ClassImpl>(sym) }.script();
  return Script{};
}

Symbol::Symbol(const Class & c)
  : Symbol(c.impl())
{

}

Symbol::Symbol(const Namespace & n)
  : Symbol(n.impl())
{

}

Symbol::Symbol(const std::shared_ptr<SymbolImpl> & impl)
  : d(impl)
{

}

Engine* Symbol::engine() const
{
  if (isClass())
    return toClass().engine();
  else if (isNamespace())
    return toNamespace().engine();
  return nullptr;
}

bool Symbol::isClass() const
{
  return dynamic_cast<ClassImpl*>(d.get()) != nullptr;
}

Class Symbol::toClass() const
{
  return Class{ std::dynamic_pointer_cast<ClassImpl>(d) };
}

bool Symbol::isNamespace() const
{
  return dynamic_cast<NamespaceImpl*>(d.get()) != nullptr;
}

Namespace Symbol::toNamespace() const
{
  return Namespace{ std::dynamic_pointer_cast<NamespaceImpl>(d) };
}

FunctionBuilder Symbol::Function(const std::string & name)
{
  if (isClass())
    return toClass().Method(name);
  else if (isNamespace())
    return toNamespace().Function(name);
  throw std::runtime_error{ "Cannot add function on null symbol" };
}

FunctionBuilder Symbol::Operation(OperatorName op)
{
  if (isClass())
    return toClass().Operation(op);
  else if (isNamespace())
    return toNamespace().Operation(op);
  throw std::runtime_error{ "Cannot add operator on null symbol" };
}

TypedefBuilder Symbol::Typedef(const Type & t, const std::string & name)
{
  return TypedefBuilder{ *this, name, t };
}

TypedefBuilder Symbol::Typedef(const Type & t, std::string && name)
{
  return TypedefBuilder{ *this, std::move(name), t };
}

void TypedefBuilder::create()
{
  if (this->symbol_.isClass())
    this->symbol_.toClass().impl()->typedefs.push_back(Typedef{ std::move(this->name_), this->type_ });
  else if(this->symbol_.isNamespace())
    this->symbol_.toNamespace().impl()->typedefs.push_back(Typedef{ std::move(this->name_), this->type_ });

  /// TODO ? Should we throw on error ?
}

} // namespace script
