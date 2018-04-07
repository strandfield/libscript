// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_SCOPE_P_H
#define LIBSCRIPT_SCOPE_P_H

#include "script/scope.h"

#include "script/class.h"
#include "script/context.h"
#include "script/enum.h"
#include "script/lambda.h"
#include "script/namespace.h"
#include "script/script.h"
#include "script/typedefs.h"

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

  static const std::vector<Class> static_dummy_classes;
  static const std::vector<Enum> static_dummy_enums;
  static const std::vector<Function> static_dummy_functions;
  static const std::vector<LiteralOperator> static_dummy_literal_operators;
  static const std::vector<Namespace> static_dummy_namespaces;
  static const std::vector<Operator> static_dummy_operators;
  static const std::vector<Template> static_dummy_templates;
  static const std::map<std::string, Value> static_dummy_values;
  static const std::vector<Typedef> static_dummy_typedefs;

  virtual const std::vector<Class> & classes() const;
  virtual const std::vector<Enum> & enums() const;
  virtual const std::vector<Function> & functions() const;
  virtual const std::vector<LiteralOperator> & literal_operators() const;
  virtual const std::vector<Namespace> & namespaces() const;
  virtual const std::vector<Operator> & operators() const;
  virtual const std::vector<Template> & templates() const;
  virtual const std::map<std::string, Value> & values() const;
  virtual const std::vector<Typedef> & typedefs() const;


  virtual void add_class(const Class & c) { throw std::runtime_error{ "Bad call to ScopeImpl::add_class()" }; }
  virtual void add_function(const Function & f) { throw std::runtime_error{ "Bad call to ScopeImpl::add_function()" }; }
  virtual void add_operator(const Operator & op) { throw std::runtime_error{ "Bad call to ScopeImpl::add_operator()" }; }
  virtual void add_literal_operator(const LiteralOperator & lo) { throw std::runtime_error{ "Bad call to ScopeImpl::add_literal_operator()" }; }
  virtual void add_cast(const Cast & c) { throw std::runtime_error{ "Bad call to ScopeImpl::add_cast()" }; }
  virtual void add_enum(const Enum & e) { throw std::runtime_error{ "Bad call to ScopeImpl::add_enum()" }; }
  virtual void add_typedef(const Typedef & td) { throw std::runtime_error{ "Bad call to ScopeImpl::add_typedef()" }; }

  virtual void remove_class(const Class & c) { throw std::runtime_error{ "Bad call to ScopeImpl::remove_class()" }; }
  virtual void remove_function(const Function & f) { throw std::runtime_error{ "Bad call to ScopeImpl::remove_function()" }; }
  virtual void remove_operator(const Operator & op) { throw std::runtime_error{ "Bad call to ScopeImpl::remove_operator()" }; }
  virtual void remove_cast(const Cast & c) { throw std::runtime_error{ "Bad call to ScopeImpl::remove_cast()" }; }
  virtual void remove_enum(const Enum & e) { throw std::runtime_error{ "Bad call to ScopeImpl::remove_enum()" }; }

  virtual bool lookup(const std::string & name, NameLookupImpl *nl) const;
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
  const std::map<std::string, Value> & values() const override;
  const std::vector<Typedef> & typedefs() const override;

  void add_class(const Class & c) override;
  void add_function(const Function & f) override;
  void add_operator(const Operator & op) override;
  void add_literal_operator(const LiteralOperator & lo) override;
  void add_enum(const Enum & e) override;
  void add_typedef(const Typedef & td) override;
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
  const std::vector<Typedef> & typedefs() const override;

  void add_class(const Class & c) override;
  void add_function(const Function & f) override;
  void add_operator(const Operator & op) override;
  void add_cast(const Cast & c) override;
  void add_enum(const Enum & e) override;
  void add_typedef(const Typedef & td) override;

  bool lookup(const std::string & name, NameLookupImpl *nl) const override;
};

class LambdaScope : public ScopeImpl
{
public:
  LambdaScope(const Lambda & l, std::shared_ptr<ScopeImpl> p = nullptr);
  ~LambdaScope() = default;

  Lambda mClosure;

  Engine * engine() const override;
  int kind() const override;

  bool lookup(const std::string & name, NameLookupImpl *nl) const override;
};

class EnumScope : public ScopeImpl
{
public:
  EnumScope(const Enum & e, std::shared_ptr<ScopeImpl> p = nullptr);
  ~EnumScope() = default;

  Enum mEnum;

  Engine * engine() const override;
  int kind() const override;

  bool lookup(const std::string & name, NameLookupImpl *nl) const override;
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
  const std::map<std::string, Value> & values() const override;
  const std::vector<Typedef> & typedefs() const override;

  void add_class(const Class & c) override;
  void add_function(const Function & f) override;
  void add_operator(const Operator & op) override;
  void add_literal_operator(const LiteralOperator & lo) override;
  void add_enum(const Enum & e) override;
  void add_typedef(const Typedef & td) override;

  void remove_class(const Class & c) override;
  void remove_enum(const Enum & e) override;

  bool lookup(const std::string & name, NameLookupImpl *nl) const override;
};

class ContextScope : public ScopeImpl
{
public:
  ContextScope(const Context & c, std::shared_ptr<ScopeImpl> p = nullptr);
  ~ContextScope() = default;

  Context mContext;

  Engine * engine() const override;
  int kind() const override;

  bool lookup(const std::string & name, NameLookupImpl *nl) const override;
};

} // namespace script

#endif // LIBSCRIPT_SCOPE_P_H
