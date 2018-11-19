// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/symbol.h"

#include "script/class.h"
#include "script/classbuilder.h"
#include "script/enumbuilder.h"
#include "script/functionbuilder.h"
#include "script/name.h"
#include "script/namespace.h"
#include "script/operatorbuilder.h"
#include "script/private/class_p.h"
#include "script/private/namespace_p.h"
#include "script/private/script_p.h"
#include "script/script.h"
#include "script/templatebuilder.h"
#include "script/typedefs.h"

namespace script
{

Symbol::Symbol(const script::Class & c)
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
  return script::Class{ std::dynamic_pointer_cast<ClassImpl>(d) };
}

bool Symbol::isNamespace() const
{
  return dynamic_cast<NamespaceImpl*>(d.get()) != nullptr;
}

Namespace Symbol::toNamespace() const
{
  return Namespace{ std::dynamic_pointer_cast<NamespaceImpl>(d) };
}

Name Symbol::name() const
{
  return d->get_name();
}

Symbol Symbol::parent() const
{
  return Symbol{ d->enclosing_symbol.lock() };
}

Script Symbol::script() const
{
  if (isNull())
    return Script{};

  if (dynamic_cast<ScriptImpl*>(d.get()) != nullptr)
    return Script{ std::static_pointer_cast<ScriptImpl>(d) };

  return parent().script();
}

ClassBuilder Symbol::newClass(const std::string & name)
{
  return ClassBuilder{ *this, name };
}

ClassBuilder Symbol::newClass(std::string && name)
{
  return ClassBuilder{ *this, std::move(name) };
}

ClassTemplateBuilder Symbol::newClassTemplate(const std::string & name)
{
  return ClassTemplateBuilder{ *this, name };
}

ClassTemplateBuilder Symbol::newClassTemplate(std::string && name)
{
  return ClassTemplateBuilder{ *this, std::move(name) };
}

EnumBuilder Symbol::newEnum(std::string && name)
{
  return EnumBuilder{ *this, std::move(name) };
}

FunctionBuilder Symbol::newFunction(const std::string & name)
{
  if (isClass())
    return toClass().newMethod(name);
  else if (isNamespace())
    return toNamespace().newFunction(name);
  throw std::runtime_error{ "Cannot add function on null symbol" };
}

FunctionTemplateBuilder Symbol::newFunctionTemplate(const std::string & name)
{
  return FunctionTemplateBuilder{ *this, name };
}

FunctionTemplateBuilder Symbol::newFunctionTemplate(std::string && name)
{
  return FunctionTemplateBuilder{ *this, std::move(name) };
}

OperatorBuilder Symbol::newOperator(OperatorName op)
{
  if (isClass())
    return toClass().newOperator(op);
  else if (isNamespace())
    return toNamespace().newOperator(op);
  throw std::runtime_error{ "Cannot add operator on null symbol" };
}

TypedefBuilder Symbol::newTypedef(const Type & t, const std::string & name)
{
  return TypedefBuilder{ *this, name, t };
}

TypedefBuilder Symbol::newTypedef(const Type & t, std::string && name)
{
  return TypedefBuilder{ *this, std::move(name), t };
}

} // namespace script
