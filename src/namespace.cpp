// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/namespace.h"
#include "script/private/namespace_p.h"

#include "script/private/class_p.h"
#include "script/engine.h"
#include "script/private/enum_p.h"
#include "script/functionbuilder.h"

namespace script
{

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
  return d->script.lock() != nullptr;
}

Script Namespace::script() const
{
  return Script{ d->script.lock() };
}

const std::string & Namespace::name() const
{
  return d->name;
}

Enum Namespace::newEnum(const std::string & name)
{
  Enum e = engine()->newEnum(name);
  d->enums.push_back(e);
  e.implementation()->enclosing_namespace = d;
  return e;
}

Function Namespace::newFunction(const FunctionBuilder & opts)
{
  script::Function f = engine()->newFunction(opts);
  if (f.isOperator())
    d->operators.push_back(f.toOperator());
  else if (f.isCast() || f.isMemberFunction() || f.isConstructor() || f.isDestructor())
    throw std::runtime_error{ "Invalid function at namespace scope" };
  else
    d->functions.push_back(f);

  return f;
}

Class Namespace::newClass(const ClassBuilder & opts)
{
  Class cla = engine()->newClass(opts);
  d->classes.push_back(cla);
  cla.implementation()->enclosing_namespace = d;
  return cla;
}

Namespace Namespace::newNamespace(const std::string & name)
{
  auto impl = std::make_shared<NamespaceImpl>(name, engine());
  impl->parent = d;
  Namespace ret{ impl };
  d->namespaces.push_back(ret);
  return ret;
}

Operator Namespace::newOperator(const FunctionBuilder & opts)
{
  if (opts.kind != Function::OperatorFunction)
    throw std::runtime_error{ "Provided FunctionBuilder cannot be used to build an Operator" };

  Operator op = engine()->newFunction(opts).toOperator();
  d->operators.push_back(op);
  return op;
}

void Namespace::addValue(const std::string & name, const Value & val)
{
  d->variables[name] = val;
}

void Namespace::addOperator(const Operator & op)
{
  d->operators.push_back(op);
}

void Namespace::addTemplate(const Template & t)
{
  d->templates.push_back(t);
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
  return Namespace{ d->parent.lock() };
}

Class Namespace::findClass(const std::string & name) const
{
  for (const auto & c : d->classes)
  {
    if (c.name() == name)
      return c;
  }

  return Class{};
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

FunctionBuilder Namespace::Function(const std::string & name, NativeFunctionSignature func) const
{
  FunctionBuilder builder{ *this };
  builder.name = name;
  builder.callback = func;
  return builder;
}

FunctionBuilder Namespace::Operation(Operator::BuiltInOperator op, NativeFunctionSignature func) const
{
  FunctionBuilder builder{ *this, op };
  builder.callback = func;
  return builder;
}

Engine * Namespace::engine() const
{
  return d->engine;
}

NamespaceImpl * Namespace::implementation() const
{
  return d.get();
}

std::weak_ptr<NamespaceImpl> Namespace::weakref() const
{
  return std::weak_ptr<NamespaceImpl>(d);
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
