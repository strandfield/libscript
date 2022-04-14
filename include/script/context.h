// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_CONTEXT_H
#define LIBSCRIPT_CONTEXT_H

#include <map>
#include <memory>
#include <string>

#include "script/value.h"

namespace script
{

class Module;
class Scope;
class Script;

class ContextImpl;

/*!
 * \class Context
 * \brief represents the context of evaluation of an expression
 * 
 * This class represents the context of Engine::eval().
 * 
 * The default constructor constructs a null context (see isNull()).
 * Calling any other function than isNull() on a null context 
 * is undefined behavior.
 * 
 * Use addVar() to add variables to the context and clear() to remove 
 * to clear the context.
 * 
 * Use exists() to test for the existence of a variable and 
 * get() to retrieve the value of a variable.
 * 
 */

class LIBSCRIPT_API Context
{
public:
  Context() = default;
  Context(const Context&) = default;
  ~Context() = default;

  explicit Context(const std::shared_ptr<ContextImpl>& impl);

  int id() const;
  bool isNull() const;

  Engine* engine() const;

  const std::string& name() const;
  void setName(const std::string& name);

  const std::map<std::string, Value>& vars() const;
  void addVar(const std::string& name, const Value& val);
  bool exists(const std::string& name) const;
  Value get(const std::string& name) const;

  void use(const Module &m);
  void use(const Script &s);
  Scope scope() const;

  void clear();

  inline const std::shared_ptr<ContextImpl>& impl() const { return d; }

private:
  std::shared_ptr<ContextImpl> d;
};

/*!
 * \endclass
 */

} // namespace script

#endif // LIBSCRIPT_CONTEXT_H
