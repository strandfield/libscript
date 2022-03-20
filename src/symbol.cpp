// Copyright (C) 2018-2022 Vincent Chambrin
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

/*!
 * \class Symbol
 */

/*!
 * \fn explicit Symbol(const Class& c)
 * \brief constructs a symbol from a class
 */
Symbol::Symbol(const script::Class & c)
  : Symbol(c.impl())
{

}

/*!
 * \fn explicit Symbol(const Namespace& n)
 * \brief constructs a symbol from a namespace
 */
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

/*!
 * \fn bool isClass() const
 * \brief returns whether the symbol is a class
 */
bool Symbol::isClass() const
{
  return dynamic_cast<ClassImpl*>(d.get()) != nullptr;
}

/*!
 * \fn Class toClass() const
 * \brief retrieves the symbol as a class
 */
Class Symbol::toClass() const
{
  return script::Class{ std::dynamic_pointer_cast<ClassImpl>(d) };
}

/*!
 * \fn bool isNamespace() const
 * \brief returns whether the symbol is a namespace
 */
bool Symbol::isNamespace() const
{
  return dynamic_cast<NamespaceImpl*>(d.get()) != nullptr;
}

/*!
 * \fn Namespace toNamespace() const
 * \brief retrieves the symbol as a namespace
 */
Namespace Symbol::toNamespace() const
{
  return Namespace{ std::dynamic_pointer_cast<NamespaceImpl>(d) };
}

Name Symbol::name() const
{
  return d->get_name();
}

/*!
 * \fn Symbol parent() const
 * \brief returns this symbol's parent
 */
Symbol Symbol::parent() const
{
  return Symbol{ d->enclosing_symbol.lock() };
}

/*!
 * \fn Script script() const
 * \brief returns the script in which the symbol is defined
 */
Script Symbol::script() const
{
  if (isNull())
    return Script{};

  if (dynamic_cast<ScriptImpl*>(d.get()) != nullptr)
    return Script{ std::static_pointer_cast<ScriptImpl>(d) };

  return parent().script();
}

/*!
 * \fn Attributes attributes() const
 * \brief returns the attributes associated to the symbol
 */
Attributes Symbol::attributes() const
{
  return script().getAttributes(*this);
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
    return FunctionBuilder::Fun(toClass(), name);
  else if (isNamespace())
    return FunctionBuilder::Fun(toNamespace(), name);
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

FunctionBuilder Symbol::newOperator(OperatorName op)
{
  if (isClass() || isNamespace())
    return FunctionBuilder(*this, SymbolKind::Operator, op);
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

/*!
 * \endclass
 */

} // namespace script
