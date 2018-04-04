// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_SCOPE_P_H
#define LIBSCRIPT_SCOPE_P_H

#include "script/scope.h"

#include "script/class.h"
#include "script/enum.h"
#include "script/namespace.h"
#include "script/script.h"

namespace script
{

class ScopeImpl
{
public:
  ScopeImpl(std::shared_ptr<ScopeImpl> p = nullptr);
  virtual ~ScopeImpl() = default;

  std::shared_ptr<ScopeImpl> parent;

  virtual Engine * engine() const = 0;
  virtual int kind() const = 0;

  virtual const std::vector<Class> & classes() const = 0;
  virtual const std::vector<Enum> & enums() const = 0;
  virtual const std::vector<Function> & functions() const = 0;
  virtual const std::vector<LiteralOperator> & literal_operators() const = 0;
  virtual const std::vector<Namespace> & namespaces() const = 0;
  virtual const std::vector<Operator> & operators() const = 0;
  virtual const std::vector<Template> & templates() const = 0;

  virtual void add_class(const Class & c) { throw std::runtime_error{ "Bad call to ScopeImpl::add_class()" }; }
  virtual void add_function(const Function & f) { throw std::runtime_error{ "Bad call to ScopeImpl::add_function()" }; }
  virtual void add_operator(const Operator & op) { throw std::runtime_error{ "Bad call to ScopeImpl::add_operator()" }; }
  virtual void add_literal_operator(const LiteralOperator & lo) { throw std::runtime_error{ "Bad call to ScopeImpl::add_literal_operator()" }; }
  virtual void add_cast(const Cast & c) { throw std::runtime_error{ "Bad call to ScopeImpl::add_cast()" }; }
  virtual void add_enum(const Enum & e) { throw std::runtime_error{ "Bad call to ScopeImpl::add_enum()" }; }

  virtual void remove_class(const Class & c) { throw std::runtime_error{ "Bad call to ScopeImpl::remove_class()" }; }
  virtual void remove_function(const Function & f) { throw std::runtime_error{ "Bad call to ScopeImpl::remove_function()" }; }
  virtual void remove_operator(const Operator & op) { throw std::runtime_error{ "Bad call to ScopeImpl::remove_operator()" }; }
  virtual void remove_cast(const Cast & c) { throw std::runtime_error{ "Bad call to ScopeImpl::remove_cast()" }; }
  virtual void remove_enum(const Enum & e) { throw std::runtime_error{ "Bad call to ScopeImpl::remove_enum()" }; }
};

class NamespaceScope : public ScopeImpl
{
public:
  NamespaceScope(const Namespace & ns, std::shared_ptr<ScopeImpl> p = nullptr);
  ~NamespaceScope() = default;

  Namespace mNamespace;

  Engine * engine() const override;
  int kind() const override;

  const std::vector<Class> & classes() const override;
  const std::vector<Enum> & enums() const override;
  const std::vector<Function> & functions() const override;
  const std::vector<LiteralOperator> & literal_operators() const override;
  const std::vector<Namespace> & namespaces() const override;
  const std::vector<Operator> & operators() const override;
  const std::vector<Template> & templates() const override;

  void add_class(const Class & c) override;
  void add_function(const Function & f) override;
  void add_operator(const Operator & op) override;
  void add_literal_operator(const LiteralOperator & lo) override;
  void add_enum(const Enum & e) override;
};

class ClassScope : public ScopeImpl
{
public:
  ClassScope(const Class & c, std::shared_ptr<ScopeImpl> p = nullptr);
  ~ClassScope() = default;

  Class mClass;

  Engine * engine() const override;
  int kind() const override;

  const std::vector<Class> & classes() const override;
  const std::vector<Enum> & enums() const override;
  const std::vector<Function> & functions() const override;
  const std::vector<Operator> & operators() const override;
  const std::vector<Template> & templates() const override;

  static const std::vector<LiteralOperator> static_dummy_literal_operators;
  const std::vector<LiteralOperator> & literal_operators() const override;

  static const std::vector<Namespace> static_dummy_namespaces;
  const std::vector<Namespace> & namespaces() const override;

  void add_class(const Class & c) override;
  void add_function(const Function & f) override;
  void add_operator(const Operator & op) override;
  void add_cast(const Cast & c) override;
  void add_enum(const Enum & e) override;
};

class EnumScope : public ScopeImpl
{
public:
  EnumScope(const Enum & e, std::shared_ptr<ScopeImpl> p = nullptr);
  ~EnumScope() = default;

  Enum mEnum;

  Engine * engine() const override;
  int kind() const override;

  static const std::vector<Class> static_dummy_classes;
  const std::vector<Class> & classes() const override;

  static const std::vector<Enum> static_dummy_enums;
  const std::vector<Enum> & enums() const override;

  static const std::vector<Function> static_dummy_functions;
  const std::vector<Function> & functions() const override;

  static const std::vector<LiteralOperator> static_dummy_literal_operators;
  const std::vector<LiteralOperator> & literal_operators() const override;

  static const std::vector<Namespace> static_dummy_namespaces;
  const std::vector<Namespace> & namespaces() const override;

  static const std::vector<Operator> static_dummy_operators;
  const std::vector<Operator> & operators() const override;

  static const std::vector<Template> static_dummy_templates;
  const std::vector<Template> & templates() const override;
};

class ScriptScope : public ScopeImpl
{
public:
  ScriptScope(const Script & s, std::shared_ptr<ScopeImpl> p = nullptr);
  ~ScriptScope() = default;

  Script mScript;

  Engine * engine() const override;
  int kind() const override;

  const std::vector<Class> & classes() const override;
  const std::vector<Enum> & enums() const override;
  const std::vector<Function> & functions() const override;
  const std::vector<LiteralOperator> & literal_operators() const override;
  const std::vector<Namespace> & namespaces() const override;
  const std::vector<Operator> & operators() const override;
  const std::vector<Template> & templates() const override;

  void add_class(const Class & c) override;
  void add_function(const Function & f) override;
  void add_operator(const Operator & op) override;
  void add_literal_operator(const LiteralOperator & lo) override;
  void add_enum(const Enum & e) override;

  void remove_class(const Class & c) override;
  void remove_enum(const Enum & e) override;
};

} // namespace script

#endif // LIBSCRIPT_SCOPE_P_H
