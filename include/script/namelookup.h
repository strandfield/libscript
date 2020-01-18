// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_NAMELOOKUP_H
#define LIBSCRIPT_NAMELOOKUP_H

/// TODO: should we forward declare Scope also ?
#include "script/scope.h"

#include "script/ast/forwards.h"

namespace script
{

class Class;
class Enumerator;
class NameLookupImpl;
class StaticDataMember;
class Template;
class Value;

class NameLookupOptions
{
public:
  NameLookupOptions();
  NameLookupOptions(const NameLookupOptions&) = default;
  ~NameLookupOptions() = default;

  enum TemplateInstantiationPolicy
  {
    IgnoreTemplateArguments = 1,
  };

  NameLookupOptions(TemplateInstantiationPolicy tip);

  bool test(TemplateInstantiationPolicy flag) const;
  void set(TemplateInstantiationPolicy flag, bool on = true);

  NameLookupOptions& operator=(const NameLookupOptions&) = default;

private:
  uint16_t d;
};

class LIBSCRIPT_API NameLookup
{
public:
  NameLookup() = default;
  NameLookup(const NameLookup &) = default;
  ~NameLookup() = default;

  explicit NameLookup(const std::shared_ptr<NameLookupImpl> & impl);

  inline bool isNull() const { return d == nullptr; }

  const Scope & scope() const;
  const std::shared_ptr<ast::Identifier> & identifier() const;

  NameLookupOptions options() const;

  enum ResultType {
    UnknownName,
    FunctionName,
    TemplateName,
    TypeName,
    VariableName,
    DataMemberName, 
    StaticDataMemberName,
    GlobalName, 
    LocalName,
    EnumValueName,
    NamespaceName,
    CaptureName,
    TemplateParameterName,
  };

  ResultType resultType() const;

  const std::vector<Function> & functions() const;
  const Type & typeResult() const;
  const Value & variable() const;
  const Template & classTemplateResult() const;
  int captureIndex() const;
  int dataMemberIndex() const;
  int globalIndex() const;
  int localIndex() const;
  int templateParameterIndex() const;
  const Enumerator & enumeratorResult() const;
  const Scope & scopeResult() const;
  const StaticDataMember & staticDataMemberResult() const;
  const Class & memberOf() const;

  static NameLookup resolve(const std::shared_ptr<ast::Identifier> & name, const Scope & scope);
  static NameLookup resolve(const std::string & name, const Scope & scope);
  static NameLookup resolve(OperatorName op, const Scope & scope);

  static NameLookup resolve(const std::shared_ptr<ast::Identifier> & name, const Scope &scp, NameLookupOptions opts);
  
  static NameLookup member(const std::string & name, const Class & cla);

  static std::vector<Function> resolve(OperatorName op, const Type & arg, const Scope & scp);
  static std::vector<Function> resolve(OperatorName op, const Type & lhs, const Type & rhs, const Scope & scp);

  NameLookup & operator=(const NameLookup &) = default;

  inline const std::shared_ptr<NameLookupImpl> & impl() const { return d; }

protected:
  bool checkBuiltinName();
  void process();
  void qualified_lookup(const std::shared_ptr<ast::Identifier> & name, const Scope & scp);
  Scope qualified_scope_lookup(const std::shared_ptr<ast::Identifier> & name, const Scope & scope);
  Scope unqualified_scope_lookup(const std::shared_ptr<ast::Identifier> & name, const Scope & scope);
  static void recursive_member_lookup(NameLookupImpl *result, const std::string & name, const Class & cla);

protected:
  std::shared_ptr<NameLookupImpl> d;
};

} // namespace script

#endif // LIBSCRIPT_NAMELOOKUP_H
