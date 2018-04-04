// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_SCOPE_H
#define LIBSCRIPT_SCOPE_H

#include "libscriptdefs.h"
#include "function.h"
#include "operator.h"

namespace script
{

class ScopeImpl;

class Enum;
class Namespace;
class Template;

class LIBSCRIPT_API Scope
{
public:
  Scope() = default;
  Scope(const Scope & other) = default;
  ~Scope() = default;
  
  Scope(const Enum & e, const Scope & parent = Scope());
  Scope(const Class & cla, const Scope & parent = Scope());
  Scope(const Namespace & na, const Scope & parent = Scope());
  Scope(const Script & s, const Scope & parent = Scope());
  Scope(const std::shared_ptr<ScopeImpl> & impl);

  enum Type {
    InvalidScope,
    ClassScope,
    NamespaceScope,
    ScriptScope,
    EnumClassScope,
  };

  bool isNull() const;
  Type type() const;

  Engine * engine() const;
  
  bool hasParent() const;
  Scope parent() const;

  bool isClass() const;

  Class asClass() const;
  Namespace asNamespace() const;
  Enum asEnum() const;
  Script asScript() const;

  const std::vector<Class> & classes() const;
  const std::vector<Enum> & enums() const;
  const std::vector<Function> & functions() const;
  const std::vector<Namespace> & namespaces() const;
  const std::vector<Operator> & operators() const;
  std::vector<Function> operators(Operator::BuiltInOperator op) const;
  const std::vector<LiteralOperator> & literalOperators() const;
  const std::vector<Template> & templates() const;
 
  Scope child(const std::string & name) const;

  static Scope find(const Class & c);
  static Scope find(const Enum & entity);

  const std::shared_ptr<ScopeImpl> & impl() const;

private:
  std::shared_ptr<ScopeImpl> d;
};

} // namespace script




#endif // LIBSCRIPT_SCOPE_H
