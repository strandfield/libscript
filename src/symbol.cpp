// Copyright (C) 2018-2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/symbol.h"

#include "script/class.h"
#include "script/name.h"
#include "script/namespace.h"
#include "script/private/class_p.h"
#include "script/private/function_p.h"
#include "script/private/namespace_p.h"
#include "script/private/script_p.h"
#include "script/script.h"
#include "script/typedefs.h"

namespace script
{

bool SymbolImpl::is_function() const
{
  return false;
}

void add_function_to_symbol(const Function& func, Symbol& parent)
{
  /// The following could be done here just in case 
  /// but is currently not necessary.
  //func.impl()->enclosing_symbol = parent.impl();

  if (parent.isClass())
  {
    Class cla = parent.toClass();
    cla.addFunction(func);
  }
  else if (parent.isNamespace())
  {
    Namespace ns = parent.toNamespace();
    ns.addFunction(func);
  }
}

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

/*!
 * \fn Symbol(const Function& f)
 * \brief constructs a symbol from a function
 */
Symbol::Symbol(const Function& f)
  : Symbol(std::shared_ptr<SymbolImpl>(f.impl()))
{

}

Symbol::Symbol(const std::shared_ptr<SymbolImpl> & impl)
  : d(impl)
{

}

/*!
 * \fn Engine* engine() const
 * \brief returns the script engine
 */
Engine* Symbol::engine() const
{
  return d->engine;
}

/*!
 * \fn Symbol::Kind kind() const
 * \brief returns the symbol's kind
 */
Symbol::Kind Symbol::kind() const
{
  return d ? d->get_kind() : SymbolKind::NotASymbol;
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

/*!
 * \fn bool isFunction() const
 * \brief returns whether the symbol is a function
 */
bool Symbol::isFunction() const
{
  return d && d->is_function();
}

/*!
 * \fn Function toFunction() const
 * \brief retrieves the symbol as a function
 */
Function Symbol::toFunction() const
{
  return Function(isFunction() ? std::static_pointer_cast<FunctionImpl>(d) : nullptr);
}

/*!
 * \fn Name name() const
 * \brief returns the symbol's name
 */
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

/*!
 * \endclass
 */

} // namespace script
