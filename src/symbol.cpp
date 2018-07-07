// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/class.h"
#include "script/namespace.h"
#include "script/private/class_p.h"
#include "script/private/namespace_p.h"
#include "script/private/script_p.h"
#include "script/symbol.h"


namespace script
{

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


} // namespace script
