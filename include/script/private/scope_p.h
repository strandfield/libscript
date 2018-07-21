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
#include "script/namespacealias.h"
#include "script/script.h"
#include "script/typedefs.h"

namespace script
{

class ScopeImpl
{
public:
  ScopeImpl(std::shared_ptr<ScopeImpl> p = nullptr);
  ScopeImpl(const ScopeImpl & other);
  virtual ~ScopeImpl() = default;

  std::shared_ptr<ScopeImpl> parent;

  virtual Engine * engine() const = 0;
  virtual int kind() const = 0;
  virtual ScopeImpl * clone() const = 0;

  bool handle_injections() const;
  void inject(const NameLookupImpl *nl);
  void inject(const std::string & name, const Type & t);

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

  virtual bool lookup(const std::string & name, NameLookupImpl *nl) const;

  virtual void invalidate_cache();
};

class ExtensibleScope : public ScopeImpl
{
public:
  ExtensibleScope(std::shared_ptr<ScopeImpl> p = nullptr);
  ExtensibleScope(const ExtensibleScope & other);
  ~ExtensibleScope() = default;

  std::map<std::string, Type> type_aliases;

  std::vector<Class> injected_classes;
  std::vector<Enum> injected_enums;
  std::vector<Function> injected_functions;
  std::map<std::string, Value> injected_values;
  std::vector<Typedef> injected_typedefs;

  bool lookup(const std::string & name, NameLookupImpl *nl) const override;
};

class NamespaceScope : public ExtensibleScope
{
public:
  NamespaceScope(const Namespace & ns, std::shared_ptr<ScopeImpl> p = nullptr);
  NamespaceScope(const NamespaceScope & other);
  ~NamespaceScope() = default;

  Namespace mNamespace;
  std::vector<Namespace> mImportedNamespaces;
  std::map<std::string, NamespaceAlias> mNamespaceAliases;

  Engine * engine() const override;
  int kind() const override;
  NamespaceScope * clone() const override;

  inline const std::string & name() const { return mNamespace.name(); }

  static std::shared_ptr<NamespaceScope> child_scope(const std::shared_ptr<NamespaceScope> & that, const std::string & name);
 
  std::shared_ptr<NamespaceScope> child_scope(const std::string & name) const;

  bool has_child(const std::string & name) const;

  mutable std::vector<Class> mClasses;
  mutable std::vector<Enum> mEnums;
  mutable std::vector<Function> mFunctions;
  mutable std::vector<LiteralOperator> mLiteralOperators;
  mutable std::vector<Operator> mOperators;
  mutable std::vector<Template> mTemplates;
  mutable std::map<std::string, Value> mValues;
  mutable std::vector<Typedef> mTypedefs;

  const std::vector<Class> & classes() const override;
  const std::vector<Enum> & enums() const override;
  const std::vector<Function> & functions() const override;
  const std::vector<LiteralOperator> & literal_operators() const override;
  const std::vector<Namespace> & namespaces() const override;
  const std::vector<Operator> & operators() const override;
  const std::vector<Template> & templates() const override;
  const std::map<std::string, Value> & values() const override;
  const std::vector<Typedef> & typedefs() const override;

  void import_namespace(const NamespaceScope & other);

  bool lookup(const std::string & name, NameLookupImpl *nl) const override;

  void invalidate_cache() override;
};

class ClassScope : public ExtensibleScope
{
public:
  ClassScope(const Class & c, std::shared_ptr<ScopeImpl> p = nullptr);
  ClassScope(const ClassScope & other);
  ~ClassScope() = default;

  Class mClass;
  AccessSpecifier mAccessibility; // accessibility of added members

  Engine * engine() const override;
  int kind() const override;
  ClassScope * clone() const override;

  std::shared_ptr<ClassScope> withAccessibility(AccessSpecifier aspec) const;

  const std::vector<Class> & classes() const override;
  const std::vector<Enum> & enums() const override;
  const std::vector<Function> & functions() const override;
  const std::vector<Operator> & operators() const override;
  const std::vector<Template> & templates() const override;
  const std::vector<Typedef> & typedefs() const override;

  bool lookup(const std::string & name, NameLookupImpl *nl) const override;

  static bool lookup(const std::string & name, const Class & c, NameLookupImpl *nl);
};

class LambdaScope : public ScopeImpl
{
public:
  LambdaScope(const ClosureType & l, std::shared_ptr<ScopeImpl> p = nullptr);
  LambdaScope(const LambdaScope & other);
  ~LambdaScope() = default;

  ClosureType mClosure;

  Engine * engine() const override;
  int kind() const override;
  LambdaScope * clone() const override;

  bool lookup(const std::string & name, NameLookupImpl *nl) const override;
};

class EnumScope : public ScopeImpl
{
public:
  EnumScope(const Enum & e, std::shared_ptr<ScopeImpl> p = nullptr);
  EnumScope(const EnumScope & other);
  ~EnumScope() = default;

  Enum mEnum;

  Engine * engine() const override;
  int kind() const override;
  EnumScope * clone() const override;

  bool lookup(const std::string & name, NameLookupImpl *nl) const override;
};

class ScriptScope : public NamespaceScope
{
public:
  ScriptScope(const Script & s, std::shared_ptr<ScopeImpl> p = nullptr);
  ScriptScope(const ScriptScope & other);
  ~ScriptScope() = default;

  int kind() const override;
  ScriptScope * clone() const override;

  bool lookup(const std::string & name, NameLookupImpl *nl) const override;
};

class ContextScope : public ExtensibleScope
{
public:
  ContextScope(const Context & c, std::shared_ptr<ScopeImpl> p = nullptr);
  ContextScope(const ContextScope & other);
  ~ContextScope() = default;

  Context mContext;

  Engine * engine() const override;
  int kind() const override;
  ContextScope * clone() const override;

  bool lookup(const std::string & name, NameLookupImpl *nl) const override;
};

} // namespace script

#endif // LIBSCRIPT_SCOPE_P_H
