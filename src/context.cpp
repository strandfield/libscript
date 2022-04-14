// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/context.h"
#include "script/private/context_p.h"

#include "script/module.h"
#include "script/script.h"

#include "script/private/scope_p.h"

namespace script
{

/*!
 * \class Context
 */

Context::Context(const std::shared_ptr<ContextImpl>& impl)
  : d(impl)
{
  
}

/*!
 * \fn int id() const
 * \brief returns whether the context id
 * 
 * This function returns -1 if the context isNull().
 */
int Context::id() const
{
  return d ? -1 : d->id;
}

/*!
 * \fn bool isNull() const
 * \brief returns whether the context is null
 */
bool Context::isNull() const
{
  return d == nullptr;
}

/*!
 * \fn Engine* engine() const
 * \brief returns the context's engine
 */
Engine* Context::engine() const
{
  return d->engine;
}

/*!
 * \fn const std::string& name() const
 * \brief returns the context name
 */
const std::string& Context::name() const
{
  return d->name;
}

/*!
 * \fn void setName(const std::string& name)
 * \brief sets the context name
 */
void Context::setName(const std::string& name)
{
  d->name = name;
}

/*!
 * \fn const std::map<std::string, Value>& vars() const
 * \brief returns the variables of the context
 */
const std::map<std::string, Value>& Context::vars() const
{
  return d->variables;
}

/*!
 * \fn void addVar(const std::string& name, const Value& val)
 * \brief add a variable to the context
 */
void Context::addVar(const std::string& name, const Value& val)
{
  d->variables[name] = val;
}

/*!
 * \fn bool exists(const std::string& name)
 * \brief returns whether a variable with a given name exists in the context
 */
bool Context::exists(const std::string& name) const
{
  return d->variables.find(name) != d->variables.end();
}

/*!
 * \fn Value get(const std::string& name) const
 * \brief retrieves a value by its name
 */
Value Context::get(const std::string& name) const
{
  auto it = d->variables.find(name);
  if (it == d->variables.end())
    return Value{};
  return it->second;
}

/*!
 * \fn void use(const Module& m)
 * \brief makes the symbols of a module available in the context
 */
void Context::use(const Module& m)
{
  d->scope.merge(m.scope());
}

/*!
 * \fn void use(const Script& s)
 * \brief makes the symbols of a script available in the context
 */
void Context::use(const Script& s)
{
  d->scope.merge(Scope{ s.rootNamespace() });
}

/*!
 * \fn Scope scope() const
 * \brief returns the context's scope
 * 
 * This scope is used to resolve names within the expression 
 * passed to Engine::eval().
 */
Scope Context::scope() const
{
  return Scope{ std::make_shared<ContextScope>(*this, d->scope.impl()) };
}

/*!
 * \fn void clear()
 * \brief clears the context
 */
void Context::clear()
{
  d->variables.clear();
}

/*!
 * \endclass
 */

} // namespace script
