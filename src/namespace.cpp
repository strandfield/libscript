// Copyright (C) 2018-2021 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/namespace.h"
#include "script/private/namespace_p.h"

#include "script/engine.h"
#include "script/enumbuilder.h"
#include "script/classbuilder.h"
#include "script/module.h"
#include "script/name.h"
#include "script/script.h"

#include "script/private/class_p.h"
#include "script/private/enum_p.h"
#include "script/private/script_p.h"
#include "script/private/template_p.h"

namespace script
{

Name NamespaceImpl::get_name() const
{
  return Name(SymbolKind::Namespace, this->name);
}

Namespace::Namespace(const std::shared_ptr<NamespaceImpl> & impl)
  : d(impl)
{

}

bool Namespace::isNull() const
{
  return d == nullptr;
}

bool Namespace::isRoot() const
{
  return d->engine->rootNamespace() == *this;
}

bool Namespace::isUnnamed() const
{
  return d->name.empty();
}

bool Namespace::isScriptNamespace() const
{
  return dynamic_cast<ScriptImpl*>(d.get()) != nullptr;
}

Script Namespace::asScript() const
{
  return Script{ std::dynamic_pointer_cast<ScriptImpl>(d) };
}

Script Namespace::script() const
{
  return Symbol{ *this }.script();
}

bool Namespace::isModuleNamespace() const
{
  return d->the_module.lock() != nullptr;
}

Module Namespace::asModule() const
{
  return Module(d->the_module.lock());
}

const std::string & Namespace::name() const
{
  return d->name;
}

Namespace Namespace::getNamespace(const std::string & name)
{
  auto ret = findNamespace(name);
  if (!ret.isNull())
    return ret;

  return newNamespace(name);
}

Namespace Namespace::newNamespace(const std::string & name)
{
  auto impl = std::make_shared<NamespaceImpl>(name, engine());
  impl->enclosing_symbol = d;
  Namespace ret{ impl };
  d->namespaces.push_back(ret);
  return ret;
}

void Namespace::addValue(const std::string & name, const Value & val)
{
  d->variables[name] = val;
}

const std::map<std::string, Value> & Namespace::vars() const
{
  return d->variables;
}

const std::vector<Enum> & Namespace::enums() const
{
  return d->enums;
}

const std::vector<Function> & Namespace::functions() const
{
  return d->functions;
}

const std::vector<Operator> & Namespace::operators() const
{
  return d->operators;
}

const std::vector<LiteralOperator> & Namespace::literalOperators() const
{
  return d->literal_operators;
}

const std::vector<Class> & Namespace::classes() const
{
  return d->classes;
}

const std::vector<Namespace> & Namespace::namespaces() const
{
  return d->namespaces;
}

const std::vector<Template> & Namespace::templates() const
{
  return d->templates;
}

const std::vector<Typedef> & Namespace::typedefs() const
{
  return d->typedefs;
}

Namespace Namespace::enclosingNamespace() const
{
  return Namespace{ std::static_pointer_cast<NamespaceImpl>(d->enclosing_symbol.lock()) };
}

Class Namespace::findClass(const std::string & name) const
{
  for (const auto & c : d->classes)
  {
    if (c.name() == name)
      return c;
  }

  return {};
}

Namespace Namespace::findNamespace(const std::string & name) const
{
  for (const auto & n : d->namespaces)
  {
    if (n.name() == name)
      return n;
  }

  return Namespace{};
}

std::vector<script::Function> Namespace::findFunctions(const std::string & name) const
{
  std::vector<script::Function> ret;
  for (const auto & f : d->functions)
  {
    if (f.name() == name)
      ret.push_back(f);
  }

  return ret;
}

ClassBuilder Namespace::newClass(const std::string & name) const
{
  return ClassBuilder{ Symbol{ *this }, name };
}

EnumBuilder Namespace::newEnum(const std::string & name) const
{
  return EnumBuilder{ Symbol{*this}, name };
}

void Namespace::addFunction(const Function& f)
{
  if (f.isOperator())
    d->operators.push_back(f.toOperator());
  else if (f.isLiteralOperator())
    d->literal_operators.push_back(f.toLiteralOperator());
  else
    d->functions.push_back(f);
}

Engine * Namespace::engine() const
{
  return d->engine;
}

bool Namespace::operator==(const Namespace & other) const
{
  return d == other.d;
}

bool Namespace::operator!=(const Namespace & other) const
{
  return d != other.d;
}

} // namespace script
