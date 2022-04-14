// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/scope.h"
#include "script/private/scope_p.h"

#include "script/engine.h"
#include "script/private/class_p.h"
#include "script/private/enum_p.h"
#include "script/enumerator.h"
#include "script/private/function_p.h"
#include "script/private/namespace_p.h"
#include "script/operator.h"
#include "script/private/script_p.h"
#include "script/private/template_p.h"
#include "script/symbol.h"
#include "script/types.h"
#include "script/typesystem.h"

#include "script/namelookup.h"
#include "script/private/namelookup_p.h"

#include <algorithm>

namespace script
{

ScopeImpl::ScopeImpl(std::shared_ptr<ScopeImpl> p)
  : parent(p)
{

}

ScopeImpl::ScopeImpl(const ScopeImpl & other)
{
  if (other.parent != nullptr)
  {
    this->parent = std::shared_ptr<ScopeImpl>(other.parent->clone());
  }
}

bool ScopeImpl::handle_injections() const
{
  return dynamic_cast<const ExtensibleScope *>(this) != nullptr;
}

void ScopeImpl::inject(const NameLookupImpl *nl)
{
  ExtensibleScope *extensible = dynamic_cast<ExtensibleScope*>(this);
  if (!extensible)
  {
    if (this->parent == nullptr)
      throw std::runtime_error{ "Scope does not supprot injection" };

    if (this->parent.use_count() == 1)
    {
      this->parent->inject(nl);
    }
    else
    {
      if (this->parent->handle_injections())
      {
        this->parent = std::shared_ptr<ScopeImpl>(this->parent->clone());
      }

      this->parent->inject(nl);
    }

    return;
  }

  if (!nl->typeResult.isNull())
  {
    if (nl->typeResult.isEnumType())
    {
      extensible->injected_enums.push_back(engine()->typeSystem()->getEnum(nl->typeResult));
    }
    else if (nl->typeResult.isObjectType())
    {
      extensible->injected_classes.push_back(engine()->typeSystem()->getClass(nl->typeResult));
    }
    else
    {
      throw std::runtime_error{ "ScopeImpl::inject() : injection of type not implemented" };
    }
  }
  else if (!nl->functions.empty())
  {
    extensible->injected_functions.insert(extensible->injected_functions.end(), nl->functions.begin(), nl->functions.end());
  }
  
  /// TODO : handle values and templates
}

void ScopeImpl::inject(const std::string & name, const Type & t)
{
  ExtensibleScope *extensible = dynamic_cast<ExtensibleScope*>(this);
  if (!extensible)
  {
    if (this->parent == nullptr)
      throw std::runtime_error{ "Scope does not supprot injection" };

    if (this->parent.use_count() == 1)
    {
      this->parent->inject(name, t);
    }
    else
    {
      if (this->parent->handle_injections())
      {
        this->parent = std::shared_ptr<ScopeImpl>(this->parent->clone());
      }

      this->parent->inject(name, t);
    }

    return;
  }

  extensible->type_aliases[name] = t;
}

const std::vector<Class> ScopeImpl::static_dummy_classes = std::vector<Class>{};
const std::vector<Enum> ScopeImpl::static_dummy_enums = std::vector<Enum>{};
const std::vector<Function> ScopeImpl::static_dummy_functions = std::vector<Function>{};
const std::vector<LiteralOperator> ScopeImpl::static_dummy_literal_operators = std::vector<LiteralOperator>{};
const std::vector<Namespace> ScopeImpl::static_dummy_namespaces = std::vector<Namespace>{};
const std::vector<Operator> ScopeImpl::static_dummy_operators = std::vector<Operator>{};
const std::vector<Template> ScopeImpl::static_dummy_templates = std::vector<Template>{};
const std::map<std::string, Value> ScopeImpl::static_dummy_values = std::map<std::string, Value>{};
const std::vector<Typedef> ScopeImpl::static_dummy_typedefs = std::vector<Typedef>{};

const std::vector<Class> & ScopeImpl::classes() const
{
  return static_dummy_classes;
}

const std::vector<Enum> & ScopeImpl::enums() const
{
  return static_dummy_enums;
}

const std::vector<Function> & ScopeImpl::functions() const
{
  return static_dummy_functions;
}

const std::vector<LiteralOperator> & ScopeImpl::literal_operators() const
{
  return static_dummy_literal_operators;
}

const std::vector<Namespace> & ScopeImpl::namespaces() const
{
  return static_dummy_namespaces;
}

const std::vector<Operator> & ScopeImpl::operators() const
{
  return static_dummy_operators;
}

const std::vector<Template> & ScopeImpl::templates() const
{
  return static_dummy_templates;
}

const std::map<std::string, Value> & ScopeImpl::values() const
{
  return static_dummy_values;
}

const std::vector<Typedef> & ScopeImpl::typedefs() const
{
  return static_dummy_typedefs;
}

bool ScopeImpl::lookup(const std::string & name, NameLookupImpl *nl) const
{

  for (const auto & e : enums())
  {
    if (e.name() == name)
    {
      nl->typeResult = e.id();
      return true;
    }

    if (e.isEnumClass())
      continue;

    auto it = e.values().find(name);
    if (it != e.values().end())
    {
      nl->enumeratorResult = Enumerator{ e, it->second };
      return true;
    }
  }

  for (const auto & c : classes())
  {
    if (c.name() == name)
    {
      nl->typeResult = c.id();
      return true;
    }
  }

  for (const auto & td : typedefs())
  {
    if (td.name() == name)
    {
      nl->typeResult = td.type();
      return true;
    }
  }

  const auto & vars = values();
  auto it = vars.find(name);
  if (it != vars.end())
  {
    nl->valueResult = it->second;
    return true;
  }

  bool found_something = false;
  for (const auto & fn : functions())
  {
    if (fn.name() == name)
    {
      nl->functions.push_back(fn);
      found_something = true;
    }
  }

  for (const auto & t : templates())
  {
    if (t.name() == name)
    {
      if (t.isClassTemplate())
      {
        nl->classTemplateResult = t.asClassTemplate();
        return true;
      }
      else
      {
        nl->functionTemplateResult.push_back(t.asFunctionTemplate());
        found_something = true;
      }
    }
  }

  if (found_something)
    return true;

  return false;
}

void ScopeImpl::invalidate_cache(int)
{

}


ExtensibleScope::ExtensibleScope(std::shared_ptr<ScopeImpl> p)
  : ScopeImpl(p)
{

}

ExtensibleScope::ExtensibleScope(const ExtensibleScope & other)
  : ScopeImpl(other)
  , type_aliases(other.type_aliases)
  , injected_classes(other.injected_classes)
  , injected_enums(other.injected_enums)
  , injected_functions(other.injected_functions)
  , injected_values(other.injected_values)
  , injected_typedefs(other.injected_typedefs)
{

}

bool ExtensibleScope::lookup(const std::string & name, NameLookupImpl *nl) const
{
  {
    auto it = type_aliases.find(name);
    if (it != type_aliases.end())
    {
      nl->typeResult = it->second;
      return true;
    }
  }

  for (const auto & c : injected_classes)
  {
    if (c.name() == name)
    {
      nl->typeResult = c.id();
      return true;
    }
  }

  for (const auto & e : injected_enums)
  {
    if (e.name() == name)
    {
      nl->typeResult = e.id();
      return true;
    }
  }

  {
    auto it = injected_values.find(name);
    if (it != injected_values.end())
    {
      nl->valueResult = it->second;
      return true;
    }
  }

  for (const auto & td : injected_typedefs)
  {
    if (td.name() == name)
    {
      nl->typeResult = td.type();
      return true;
    }
  }

  const size_t size_before = nl->functions.size();
  for (const auto & f : injected_functions)
  {
    if (f.name() == name)
      nl->functions.push_back(f);
  }

  const bool found = ScopeImpl::lookup(name, nl);
  return found || (nl->functions.size() != size_before);
}


NamespaceScope::NamespaceScope(const Namespace & ns, std::shared_ptr<ScopeImpl> p)
  : ExtensibleScope(p)
  , mNamespace(ns)
{

}

NamespaceScope::NamespaceScope(const NamespaceScope & other)
  : ExtensibleScope(other)
  , mNamespace(other.mNamespace)
  , mImportedNamespaces(other.mImportedNamespaces)
  , mNamespaceAliases(other.mNamespaceAliases)
{

}

Engine * NamespaceScope::engine() const
{
  return mNamespace.engine();
}

int NamespaceScope::kind() const
{
  return Scope::NamespaceScope;
}

NamespaceScope * NamespaceScope::clone() const
{
  return new NamespaceScope(*this);
}

std::shared_ptr<NamespaceScope> NamespaceScope::child_scope(const std::shared_ptr<NamespaceScope> & that, const std::string & name)
{
  auto result = that->child_scope(name);
  if (result == nullptr)
    return result;
  result->parent = that;
  return result;
}

std::shared_ptr<NamespaceScope> NamespaceScope::child_scope(const std::string & name) const
{
  Namespace base;
  std::vector<Namespace> imported;

  if (!mNamespace.isNull())
  {
    for (const auto & ns : mNamespace.namespaces())
    {
      if (ns.name() == name)
      {
        base = ns;
        break;
      }
    }
  }

  for (const auto & ins : mImportedNamespaces)
  {
    for (const auto & ns : ins.namespaces())
    {
      if (ns.name() == name)
      {
        imported.push_back(ns);
        break;
      }
    }
  }

  if (base.isNull() && imported.empty())
    return nullptr;

  auto ret = std::make_shared<script::NamespaceScope>(base);
  ret->mImportedNamespaces = std::move(imported);
  return ret;
}

bool NamespaceScope::has_child(const std::string & name) const
{
  Namespace base;
  std::vector<Namespace> imported;

  if (!mNamespace.isNull())
  {
    for (const auto & ns : mNamespace.namespaces())
    {
      if (ns.name() == name)
        return true;
    }
  }

  for (const auto & ins : mImportedNamespaces)
  {
    for (const auto & ns : ins.namespaces())
    {
      if (ns.name() == name)
        return true;
    }
  }

  return false;
}

const std::vector<Class> & NamespaceScope::classes() const
{
  if (mImportedNamespaces.empty())
    return mNamespace.classes();

  if (mClasses.empty())
  {
    mClasses = mNamespace.isNull() ? std::vector<Class>{} : mNamespace.classes();
    for (const auto & ns : mImportedNamespaces)
      mClasses.insert(mClasses.end(), ns.classes().begin(), ns.classes().end());
  }

  return mClasses;
}

const std::vector<Enum> & NamespaceScope::enums() const
{
  if (mImportedNamespaces.empty())
    return mNamespace.enums();

  if (mEnums.empty())
  {
    mEnums = mNamespace.isNull() ? std::vector<Enum>{} : mNamespace.enums();
    for (const auto & ns : mImportedNamespaces)
      mEnums.insert(mEnums.end(), ns.enums().begin(), ns.enums().end());
  }

  return mEnums;
}

const std::vector<Function> & NamespaceScope::functions() const
{
  if (mImportedNamespaces.empty())
    return mNamespace.functions();

  if (mFunctions.empty())
  {
    mFunctions = mNamespace.isNull() ? std::vector<Function>{} : mNamespace.functions();
    for (const auto & ns : mImportedNamespaces)
      mFunctions.insert(mFunctions.end(), ns.functions().begin(), ns.functions().end());
  }

  return mFunctions;
}

const std::vector<LiteralOperator> & NamespaceScope::literal_operators() const
{
  if (mImportedNamespaces.empty())
    return mNamespace.literalOperators();

  if (mLiteralOperators.empty())
  {
    mLiteralOperators = mNamespace.isNull() ? std::vector<LiteralOperator>{} : mNamespace.literalOperators();
    for (const auto & ns : mImportedNamespaces)
      mLiteralOperators.insert(mLiteralOperators.end(), ns.literalOperators().begin(), ns.literalOperators().end());
  }

  return mLiteralOperators;
}

const std::vector<Namespace> & NamespaceScope::namespaces() const
{
  return mNamespace.namespaces();
}

const std::vector<Operator> & NamespaceScope::operators() const
{
  if (mImportedNamespaces.empty())
    return mNamespace.operators();

  if (mOperators.empty())
  {
    mOperators = mNamespace.isNull() ? std::vector<Operator>{} : mNamespace.operators();
    for (const auto & ns : mImportedNamespaces)
      mOperators.insert(mOperators.end(), ns.operators().begin(), ns.operators().end());
  }

  return mOperators;
}

const std::vector<Template> & NamespaceScope::templates() const
{
  if (mImportedNamespaces.empty())
    return mNamespace.templates();

  if (mTemplates.empty())
  {
    mTemplates = mNamespace.isNull() ? std::vector<Template>{} : mNamespace.templates();
    for (const auto & ns : mImportedNamespaces)
      mTemplates.insert(mTemplates.end(), ns.templates().begin(), ns.templates().end());
  }

  return mTemplates;
}

const std::vector<Typedef> & NamespaceScope::typedefs() const
{
  if (mImportedNamespaces.empty())
    return mNamespace.typedefs();

  if (mTypedefs.empty())
  {
    mTypedefs = mNamespace.isNull() ? std::vector<Typedef>{} : mNamespace.typedefs();
    for (const auto & ns : mImportedNamespaces)
      mTypedefs.insert(mTypedefs.end(), ns.typedefs().begin(), ns.typedefs().end());
  }

  return mTypedefs;
}

const std::map<std::string, Value> & NamespaceScope::values() const
{
  if (mImportedNamespaces.empty())
    return mNamespace.vars();

  if (mValues.empty())
  {
    mValues = mNamespace.isNull() ? std::map<std::string, Value>{} : mNamespace.vars();
    for (const auto & ns : mImportedNamespaces)
    {
      for (const auto & it : ns.vars())
      {
        mValues[it.first] = it.second;
      }
    }
  }

  return mValues;
}

void NamespaceScope::import_namespace(const NamespaceScope & other)
{
  if (!other.mNamespace.isNull())
    mImportedNamespaces.push_back(other.mNamespace);

  for (const auto & ns : other.mImportedNamespaces)
    mImportedNamespaces.push_back(ns);

  invalidate_cache(Scope::InvalidateAllCaches);
}

bool NamespaceScope::lookup(const std::string & name, NameLookupImpl *nl) const
{
  if (has_child(name))
  {
    nl->scopeResult = Scope{ child_scope(name) };
    return true;
  }

  return ExtensibleScope::lookup(name, nl);
}

void NamespaceScope::invalidate_cache(int which)
{
  if (which & Scope::InvalidateClassCache)
    mClasses.clear();
  if (which & Scope::InvalidateEnumCache)
    mEnums.clear();
  if (which & Scope::InvalidateFunctionCache)
    mFunctions.clear();
  if (which & Scope::InvalidateLiteralOperatorCache)
    mLiteralOperators.clear();
  if (which & Scope::InvalidateOperatorCache)
    mOperators.clear();
  if (which & Scope::InvalidateTemplateCache)
    mTemplates.clear();
  if (which & Scope::InvalidateVariableCache)
    mValues.clear();
  if (which & Scope::InvalidateTypedefCache)
    mTypedefs.clear();
}



ClassScope::ClassScope(const Class & c, std::shared_ptr<ScopeImpl> p)
  : ExtensibleScope(p)
  , mClass(c)
  , mAccessibility(AccessSpecifier::Private)
{

}

ClassScope::ClassScope(const ClassScope & other)
  : ExtensibleScope(other)
  , mClass(other.mClass)
  , mAccessibility(other.mAccessibility)
{

}

Engine * ClassScope::engine() const
{
  return mClass.engine();
}

int ClassScope::kind() const
{
  return Scope::ClassScope;
}

std::shared_ptr<ClassScope> ClassScope::withAccessibility(AccessSpecifier aspec) const
{
  auto cln = clone();
  cln->mAccessibility = aspec;
  return std::shared_ptr<ClassScope>(cln);
}

ClassScope * ClassScope::clone() const
{
  return new ClassScope(*this);
}

const std::vector<Class> & ClassScope::classes() const
{
  return mClass.classes();
}

const std::vector<Enum> & ClassScope::enums() const
{
  return mClass.enums();
}

const std::vector<Function> & ClassScope::functions() const
{
  return mClass.memberFunctions();
}

const std::vector<Operator> & ClassScope::operators() const
{
  return mClass.operators();
}

const std::vector<Template> & ClassScope::templates() const
{
  return mClass.templates();
}

const std::vector<Typedef> & ClassScope::typedefs() const
{
  return mClass.typedefs();
}

bool ClassScope::lookup(const std::string & name, NameLookupImpl *nl) const
{
  if (ExtensibleScope::lookup(name, nl))
    return true;

  // not really optimised, but we cannot pass the parent directly 
  // if we want to look at the static and non-static data members...
  return lookup(name, mClass, nl);
}

bool ClassScope::lookup(const std::string & name, const Class & c, NameLookupImpl *nl)
{
  if (c.isNull())
    return false;

  {
    const auto & dm = c.dataMembers();
    for (size_t i(dm.size()); i-- > 0; )
    {
      if (dm.at(i).name == name)
      {
        nl->dataMemberIndex = static_cast<int>(i) + c.attributesOffset();
        nl->memberOfResult = c;
        return true;
      }
    }
  }

  {
    const auto & sdm = c.staticDataMembers();
    auto it = sdm.find(name);
    if (it != sdm.end())
    {
      nl->staticDataMemberResult = it->second;
      nl->memberOfResult = c;
      return true;
    }
  }

  for (const auto & e : c.enums())
  {
    if (e.name() == name)
    {
      nl->typeResult = e.id();
      return true;
    }

    if (e.isEnumClass())
      continue;

    auto it = e.values().find(name);
    if (it != e.values().end())
    {
      nl->enumeratorResult = Enumerator{ e, it->second };
      return true;
    }
  }

  for (const auto & nested_class : c.classes())
  {
    if (nested_class.name() == name)
    {
      nl->typeResult = c.id();
      return true;
    }
  }

  for (const auto & td : c.typedefs())
  {
    if (td.name() == name)
    {
      nl->typeResult = td.type();
      return true;
    }
  }

  bool found_something = false;
  for (const auto & fn : c.memberFunctions())
  {
    if (fn.name() == name)
    {
      nl->functions.push_back(fn);
      found_something = true;
    }
  }

  for (const auto & t : c.templates())
  {
    if (t.name() == name)
    {
      if (t.isClassTemplate())
      {
        nl->classTemplateResult = t.asClassTemplate();
        return true;
      }
      else
      {
        nl->functionTemplateResult.push_back(t.asFunctionTemplate());
        found_something = true;
      }
    }
  }

  if (found_something)
    return true;

  return lookup(name, c.parent(), nl);
}


LambdaScope::LambdaScope(const ClosureType & l, std::shared_ptr<ScopeImpl> p)
  : ScopeImpl(p)
  , mClosure(l)
{

}

LambdaScope::LambdaScope(const LambdaScope & other)
  : ScopeImpl(other)
  , mClosure(other.mClosure)
{

}

Engine * LambdaScope::engine() const
{
  return mClosure.engine();
}

int LambdaScope::kind() const
{
  return Scope::LambdaScope;
}

LambdaScope * LambdaScope::clone() const
{
  return new LambdaScope(*this);
}

bool LambdaScope::lookup(const std::string & name, NameLookupImpl *nl) const
{
  const auto & captures = mClosure.captures();
  if (captures.empty())
    return false;

  for (size_t i(captures.size()); i-- > 0;)
  {
    if (captures.at(i).name == name)
    {
      nl->captureIndex = static_cast<int>(i);
      return true;
    }
  }

  if (captures.front().name == "this")
  {
    Class cla = engine()->typeSystem()->getClass(captures.front().type);
    const int dmi = cla.attributeIndex(name);
    if (dmi != -1)
    {
      nl->dataMemberIndex = dmi;
      return true;
    }
  }

  /// TODO : should it be ScopeImpl::lookup(name, nl) ?
  return false;
}


EnumScope::EnumScope(const Enum & e, std::shared_ptr<ScopeImpl> p)
  : ScopeImpl(p)
  , mEnum(e)
{

}

EnumScope::EnumScope(const EnumScope & other)
  : ScopeImpl(other)
  , mEnum(other.mEnum)
{

}

Engine * EnumScope::engine() const
{
  return mEnum.engine();
}

int EnumScope::kind() const
{
  return Scope::EnumClassScope;
}

EnumScope * EnumScope::clone() const
{
  return new EnumScope(*this);
}

bool EnumScope::lookup(const std::string & name, NameLookupImpl *nl) const
{
  const auto & vals = mEnum.values();
  auto it = vals.find(name);
  if (it == vals.end())
    return false;
  nl->enumeratorResult = Enumerator{ mEnum, it->second };
  return true;
}


ScriptScope::ScriptScope(const Script & s, std::shared_ptr<ScopeImpl> p)
  : NamespaceScope(s.rootNamespace(), p)
{

}

ScriptScope::ScriptScope(const ScriptScope & other)
  : NamespaceScope(other)
{

}

int ScriptScope::kind() const
{
  return Scope::ScriptScope;
}

ScriptScope * ScriptScope::clone() const
{
  return new ScriptScope(*this);
}

bool ScriptScope::lookup(const std::string & name, NameLookupImpl *nl) const
{
  const auto & globals = mNamespace.asScript().globalNames();
  auto it = globals.find(name);
  if (it != globals.end())
  {
    nl->globalIndex = it->second;
    return true;
  }

  return NamespaceScope::lookup(name, nl);
}



ContextScope::ContextScope(const Context & c, std::shared_ptr<ScopeImpl> p)
  : ExtensibleScope(p)
  , mContext(c)
{

}

ContextScope::ContextScope(const ContextScope & other)
  : ExtensibleScope(other)
  , mContext(other.mContext)
{

}

Engine * ContextScope::engine() const
{
  return mContext.engine();
}

int ContextScope::kind() const
{
  return Scope::ContextScope;
}

ContextScope * ContextScope::clone() const
{
  return new ContextScope(*this);
}

bool ContextScope::lookup(const std::string & name, NameLookupImpl *nl) const
{
  const auto & vals = mContext.vars();
  auto it = vals.find(name);
  if (it == vals.end())
    return ExtensibleScope::lookup(name, nl);

  nl->valueResult = it->second;
  return true;
}

/*!
 * \class Scope
 */

/*!
 * \fn Scope(const Enum& e, const Scope& parent = Scope())
 * \brief construct a scope from an enum
 */
Scope::Scope(const Enum& e, const Scope& parent)
{
  d = std::make_shared<script::EnumScope>(e, parent.impl());
}

/*!
 * \fn Scope(const Class& cla, const Scope& parent = Scope())
 * \brief construct a scope from a class
 */
Scope::Scope(const Class& cla, const Scope& parent)
{
  d = std::make_shared<script::ClassScope>(cla, parent.impl());
}

/*!
 * \fn Scope(const Namespace& na, const Scope& parent = Scope())
 * \brief construct a scope from a namespace
 */
Scope::Scope(const Namespace& na, const Scope& parent)
{
  d = std::make_shared<script::NamespaceScope>(na, parent.impl());
}

/*!
 * \fn Scope(const Script& s, const Scope& parent = Scope())
 * \brief construct a scope from a script
 */
Scope::Scope(const Script& s, const Scope& parent)
{
  d = std::make_shared<script::ScriptScope>(s, parent.impl());
}

Scope::Scope(const std::shared_ptr<ScopeImpl>& impl)
  : d(impl)
{

}

/*!
 * \fn bool isNull() const
 * \brief returns whether this scope is null
 */
bool Scope::isNull() const
{
  return d == nullptr;
}

/*!
 * \fn Scope::Type type() const
 * \brief returns the scope's type
 */
Scope::Type Scope::type() const
{
  if (isNull())
    return InvalidScope;
  return static_cast<Scope::Type>(d->kind());
}

/*!
 * \fn Engine* engine() const
 * \brief returns the script engine
 */
Engine* Scope::engine() const
{
  return d->engine();
}

/*!
 * \fn bool hasParent() const
 * \brief returns whether the scope as a parent scope
 */
bool Scope::hasParent() const
{
  return d && d->parent != nullptr;
}

/*!
 * \fn Scope parent() const
 * \brief returns the parent of this scope
 */
Scope Scope::parent() const
{
  return Scope(d->parent);
}

/*!
 * \fn bool isClass() const
 * \brief returns whether this scope is a class
 */
bool Scope::isClass() const
{
  return dynamic_cast<const script::ClassScope*>(d.get()) != nullptr;
}

/*!
 * \fn bool isNamespace() const
 * \brief returns whether this scope is a namespace
 */
bool Scope::isNamespace() const
{
  return dynamic_cast<const script::NamespaceScope*>(d.get()) != nullptr;
}

/*!
 * \fn bool isScript() const
 * \brief returns whether this scope is a script
 */
bool Scope::isScript() const
{
  return dynamic_cast<const script::ScriptScope*>(d.get()) != nullptr;
}

/*!
 * \fn Class asClass() const
 * \brief returns this scope as a class
 */
Class Scope::asClass() const
{
  return std::dynamic_pointer_cast<script::ClassScope>(d)->mClass;
}

/*!
 * \fn Namespace asNamespace() const
 * \brief returns this scope as a namespace
 */
Namespace Scope::asNamespace() const
{
  return std::dynamic_pointer_cast<script::NamespaceScope>(d)->mNamespace;
}

/*!
 * \fn Enum asEnum() const
 * \brief returns this scope as an enumeration
 */
Enum Scope::asEnum() const
{
  return std::dynamic_pointer_cast<script::EnumScope>(d)->mEnum;
}

/*!
 * \fn Script asScript()
 * \brief returns this scope as a script
 */
Script Scope::asScript() const
{
  return std::dynamic_pointer_cast<script::NamespaceScope>(d)->mNamespace.asScript();
}

/*!
 * \fn Symbol symbol() const
 * \brief returns the symbol that is enclosing this scope
 */
Symbol Scope::symbol() const
{
  if (isNull())
    return Symbol{};
  else if (isClass())
    return Symbol{ asClass() };
  else if (isNamespace())
    return Symbol{ asNamespace() };
  // @TODO: add the function
  return parent().symbol();
}

/*!
 * \fn Script script() const
 * \brief Returns the script that contains this scope
 */
Script Scope::script() const
{
  if (isNull())
    return Script{};
  else if (isScript())
    return asScript();
  else
    return parent().script();
}

/*!
 * \fn const std::vector<Class>& classes() const
 * \brief returns the classes in this scope
 */
const std::vector<Class>& Scope::classes() const
{
  return d->classes();
}

/*!
 * \fn const std::vector<Enum>& enums() const
 * \brief returns the enums in this scope
 */
const std::vector<Enum>& Scope::enums() const
{
  return d->enums();
}

/*!
 * \fn const std::vector<Function>& functions() const
 * \brief returns the functions in this scope
 */
const std::vector<Function>& Scope::functions() const
{
  return d->functions();
}

/*!
 * \fn const std::vector<Namespace>& namespaces() const
 * \brief returns the namespaces in this scope
 */
const std::vector<Namespace>& Scope::namespaces() const
{
  return d->namespaces();
}

/*!
 * \fn const std::vector<Operator>& operators() const
 * \brief returns the operators in this scope
 */
const std::vector<Operator>& Scope::operators() const
{
  return d->operators();
}

/*!
 * \fn const std::vector<LiteralOperator>& literalOperators() const
 * \brief returns the literal operators in this scope
 */
const std::vector<LiteralOperator>& Scope::literalOperators() const
{
  return d->literal_operators();
}

/*!
 * \fn const std::vector<Template>& templates() const
 * \brief returns the templates in this scope
 */
const std::vector<Template>& Scope::templates() const
{
  return d->templates();
}

/*!
 * \fn AccessSpecifier accessibility() const
 */
AccessSpecifier Scope::accessibility() const
{
  if (!isClass())
    return AccessSpecifier::Public;

  return std::static_pointer_cast<script::ClassScope>(impl())->mAccessibility;
}

/*!
 * \fn std::vector<Function> operators(OperatorName op) const
 * \brief get all operators of a given name in this scope
 * 
 * This function only returns operators in this scope and excludes 
 * operators from the parent scope.
 */
std::vector<Function> Scope::operators(OperatorName op) const
{
  const std::vector<Operator> & candidates = operators();
  std::vector<Function> ret;
  for (size_t i(0); i < candidates.size(); ++i)
  {
    if (candidates.at(i).operatorId() != op)
      continue;
    ret.push_back(candidates.at(i));
  }

  return ret;
}

/*!
 * \fn Scope child(const std::string& name) const
 * \brief returns a child scope of this scope
 */
Scope Scope::child(const std::string& name) const
{
  if (isNull())
    return Scope{};

  for (const auto & cla : this->classes())
  {
    if (cla.name() == name)
      return Scope{ cla, *this };
  }

  for (const auto & e : this->enums())
  {
    if (e.name() == name)
      return Scope{ e, *this };
  }

  auto nsscope = std::dynamic_pointer_cast<script::NamespaceScope>(d);
  if (nsscope != nullptr)
  {
    auto it = nsscope->mNamespaceAliases.find(name);
    if (it != nsscope->mNamespaceAliases.end())
      return this->child(it->second.nested());

    return Scope{ script::NamespaceScope::child_scope(nsscope, name) };
  }
  else
  {
    for (const auto & ns : this->namespaces())
    {
      if (ns.name() == name)
        return Scope{ ns, *this };
    }
  }

  return Scope{};
}

Scope Scope::child(const std::vector<std::string>& name) const
{
  Scope ret = *this;
  for (size_t i(0); i < name.size(); ++i)
  {
    ret = ret.child(name.at(i));
    if (ret.isNull())
      break;
  }

  return ret;
}

/*!
 * \fn void inject(const std::string& name, const script::Type& t)
 * \brief inject a type alias in this scope
 */
void Scope::inject(const std::string& name, const script::Type& t)
{
  if (d->handle_injections())
  {
    if (d.use_count() != 1)
    {
      d = std::shared_ptr<ScopeImpl>(d->clone());
    }

    d->inject(name, t);
  }
  else
  {
    d->inject(name, t);
  }
}

/*!
 * \fn void inject(const Class& cla)
 * \brief inject a class in this scope
 */
void Scope::inject(const Class& cla)
{
  NameLookupImpl lookup;
  lookup.typeResult = cla.id();
  if (d->handle_injections())
  {
    if (d.use_count() != 1)
    {
      d = std::shared_ptr<ScopeImpl>(d->clone());
    }

    d->inject(&lookup);
  }
  else
  {
    d->inject(&lookup);
  }
}

/*!
 * \fn void inject(const Enum& e)
 * \brief inject an enum in this scope
 */
void Scope::inject(const Enum& e)
{
  NameLookupImpl lookup;
  lookup.typeResult = e.id();
  if (d->handle_injections())
  {
    if (d.use_count() != 1)
    {
      d = std::shared_ptr<ScopeImpl>(d->clone());
    }

    d->inject(&lookup);
  }
  else
  {
    d->inject(&lookup);
  }
}

void Scope::inject(const NameLookupImpl *nl)
{
  if (d->handle_injections())
  {
    if (d.use_count() != 1)
    {
      d = std::shared_ptr<ScopeImpl>(d->clone());
    }

    d->inject(nl);
  }
  else
  {
    d->inject(nl);
  }
}

/*!
 * \fn void inject(const Scope& scp)
 * \brief inject a namespace scope in this scope
 */
void Scope::inject(const Scope& scp)
{
  if (!scp.isNamespace())
    throw std::runtime_error{ "Cannot inject non-namespace scope" };

  d = std::shared_ptr<ScopeImpl>(d->clone());
  auto target = d;
  while (dynamic_cast<script::NamespaceScope*>(target.get()) == nullptr)
  {
    if (target->parent == nullptr)
      throw std::runtime_error{ "Cannot inject namespace scope into non-namespace scope" };

    target = target->parent;
  }

  script::NamespaceScope *nam_scope = dynamic_cast<script::NamespaceScope*>(target.get());
  assert(nam_scope != nullptr);
  nam_scope->import_namespace(dynamic_cast<const script::NamespaceScope &>(*scp.impl()));
}

/*!
 * \fn void merge(const Scope& scp)
 * \brief merge with another scope
 * 
 * TODO: clarify difference with inject()
 */
void Scope::merge(const Scope& scp)
{
  if (!scp.parent().isNull())
    throw std::runtime_error{ "Scope::merge() : Cannot merge scope with parent" };

  d = std::shared_ptr<ScopeImpl>(d->clone());

  std::vector<std::shared_ptr<ScopeImpl>> scopes;
  {
    auto it = d;
    while (it != nullptr)
    {
      scopes.push_back(it);
      it = it->parent;
    }
    
    std::reverse(scopes.begin(), scopes.end());
  }

  auto imported = std::dynamic_pointer_cast<script::NamespaceScope>(scp.impl());
  
  int i = 0;
  while (i < int(scopes.size()) && dynamic_cast<script::NamespaceScope*>(scopes[i].get()) != nullptr && imported != nullptr)
  {
    dynamic_cast<script::NamespaceScope*>(scopes[i].get())->import_namespace(dynamic_cast<const script::NamespaceScope &>(*imported));
    if (i < int(scopes.size()) - 1 && dynamic_cast<script::NamespaceScope*>(scopes[i+1].get()) != nullptr)
    {
      const std::string & child_name = std::dynamic_pointer_cast<script::NamespaceScope>(scopes[i + 1])->name();
      imported = script::NamespaceScope::child_scope(imported, child_name);
    }
    ++i;
  }
}

/*!
 * \fn void inject(const NamespaceAlias& alias)
 * \brief inject a namespace alias in this scope
 */
void Scope::inject(const NamespaceAlias& alias)
{
  d = std::shared_ptr<ScopeImpl>(d->clone());
  auto target = d;
  while (target != nullptr)
  {
    if (dynamic_cast<script::NamespaceScope*>(target.get()) == nullptr)
    {
      target = target->parent;
      continue;
    }

    auto ns_scope = std::dynamic_pointer_cast<script::NamespaceScope>(target);
    if (ns_scope->has_child(alias.nested().front()))
    {
      ns_scope->mNamespaceAliases[alias.name()] = alias;
      return;
    }

    target = target->parent;
  }

  throw std::runtime_error{ "Scope::inject() : could not inject namespace alias" };
}

/*!
 * \fn static Namespace enclosingNamespace(const Type& t, Engine* e)
 * \brief Return the enclosing namespace of the given type.
 *
 */
Namespace Scope::enclosingNamespace(const script::Type& t, Engine* e)
{
  if (t.isFundamentalType() || t.isClosureType() || t.isFunctionType())
    return e->rootNamespace();
  else if (t.isObjectType())
    return e->typeSystem()->getClass(t).enclosingNamespace();
  else if (t.isEnumType())
    return e->typeSystem()->getEnum(t).enclosingNamespace();

  // Reasonable default
  return e->rootNamespace();
}

/*!
 * \fn std::vector<Function> lookup(const LiteralOperator&, const std::string& suffix) const
 * \brief lookup literal operators with a given suffix
 * 
 * This will search for literal operators in this scope, and, if none was found, in 
 * the parent scope.
 */
std::vector<Function> Scope::lookup(const LiteralOperator&, const std::string& suffix) const
{
  std::vector<Function> ret;
  for (const auto & lop : literalOperators())
  {
    if (lop.suffix() == suffix)
      ret.push_back(lop);
  }

  if (ret.empty() && hasParent())
    return parent().lookup(LiteralOperator{}, suffix);

  return ret;
}

/*!
 * \fn std::vector<Function> lookup(OperatorName op) const
 * \brief lookup operators with a given name
 *
 * This will search for operators in this scope, and, if none was found, in
 * the parent scope.
 */
std::vector<Function> Scope::lookup(OperatorName op) const
{
  std::vector<Function> ret;
  for (const auto & candidate : operators())
  {
    if (candidate.operatorId() == op)
      ret.push_back(candidate);
  }

  if (ret.empty() && hasParent())
    return parent().lookup(op);

  return ret;
}

/*!
 * \fn NameLookup lookup(const std::string& name) const
 * \brief perform name lookup in this scope
 */
NameLookup Scope::lookup(const std::string& name) const
{
  auto nl = std::make_shared<NameLookupImpl>();
  lookup(name, nl);
  return NameLookup{ nl };
}

bool Scope::lookup(const std::string & name, std::shared_ptr<NameLookupImpl> nl) const
{
  if (d->lookup(name, nl.get()))
    return true;

  if (d->parent == nullptr)
    return false;

  return Scope{ d->parent }.lookup(name, nl);
}

bool Scope::lookup(const std::string & name, NameLookupImpl *nl) const
{
  if (d->lookup(name, nl))
    return true;

  if (d->parent == nullptr)
    return false;

  return Scope{ d->parent }.lookup(name, nl);
}

void Scope::invalidateCache(int which)
{
  d->invalidate_cache(which);
}

const std::shared_ptr<ScopeImpl> & Scope::impl() const
{
  return d;
}

/*!
 * \endclass
 */

ScopeGuard::ScopeGuard(Scope & gs)
{
  guarded_scope = &gs;
  old_value = gs;
}

ScopeGuard::~ScopeGuard()
{
  *guarded_scope = old_value;
}

} // namespace script