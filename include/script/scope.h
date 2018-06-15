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

enum class AccessSpecifier;
class Enum;
class Namespace;
class NamespaceAlias;
class Template;

class NameLookup;
class NameLookupImpl;

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
    FunctionScope,
    LambdaScope,
    ContextScope,
    TemplateArgumentScope,
  };

  bool isNull() const;
  Type type() const;

  Engine * engine() const;
  
  bool hasParent() const;
  Scope parent() const;

  inline Scope escapeTemplate() const { return type() == TemplateArgumentScope ? parent() : *this; }

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

  AccessSpecifier accessibility() const;
 
  Scope child(const std::string & name) const;
  Scope child(const std::vector<std::string> & name) const;

  void inject(const std::string & name, const script::Type & t);
  void inject(const Class & cla);
  void inject(const Enum & e);
  void inject(const NameLookupImpl *nl);

  void inject(const Scope & scp); // injects a NamespaceScope

  void merge(const Scope & scp); // merges recursively this scope with scp

  void inject(const NamespaceAlias & alias);

  std::vector<Function> lookup(const LiteralOperator &, const std::string & suffix) const;
  std::vector<Function> lookup(Operator::BuiltInOperator op) const;

  NameLookup lookup(const std::string & name) const;
  bool lookup(const std::string & name, std::shared_ptr<NameLookupImpl> nl) const;
  bool lookup(const std::string & name, NameLookupImpl *nl) const;

  const std::shared_ptr<ScopeImpl> & impl() const;

private:
  std::shared_ptr<ScopeImpl> d;
};


class LIBSCRIPT_API ScopeGuard
{
public:
  ScopeGuard(Scope & gs);
  ~ScopeGuard();

private:
  Scope *guarded_scope;
  Scope old_value;
};

} // namespace script




#endif // LIBSCRIPT_SCOPE_H
