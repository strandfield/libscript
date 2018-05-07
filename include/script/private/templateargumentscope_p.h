// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TEMPLATE_ARG_SCOPE_P_H
#define LIBSCRIPT_TEMPLATE_ARG_SCOPE_P_H

#include "script/private/scope_p.h"

#include "script/template.h"

namespace script
{

class TemplateArgumentScope : public ScopeImpl
{
public:
  TemplateArgumentScope(const Template & t, const std::vector<TemplateArgument> & args);
  TemplateArgumentScope(const TemplateArgumentScope & other);
  ~TemplateArgumentScope() = default;

  Engine * engine() const override;
  int kind() const override;
  TemplateArgumentScope * clone() const override;

  bool lookup(const std::string & name, NameLookupImpl *nl) const override;

  void add_class(const Class & c) override;
  void add_function(const Function & f) override;
  void add_operator(const Operator & op) override;
  void add_literal_operator(const LiteralOperator & lo) override;
  void add_cast(const Cast & c) override;
  void add_enum(const Enum & e) override;
  void add_template(const Template & t) override;
  void add_typedef(const Typedef & td) override;

  void remove_class(const Class & c) override;
  void remove_function(const Function & f) override;
  void remove_operator(const Operator & op) override;
  void remove_cast(const Cast & c) override;
  void remove_enum(const Enum & e) override;

public:
  Template template_;
  std::vector<TemplateArgument> arguments_; /// TODO : perhaps a const std::vector<TemplateArgument> * would be enough
};

class TemplateParameterScope : public ScopeImpl
{
public:
  TemplateParameterScope(const Template & t);
  TemplateParameterScope(const TemplateParameterScope & other);
  ~TemplateParameterScope() = default;

  Engine * engine() const override;
  int kind() const override;
  TemplateParameterScope * clone() const override;

  bool lookup(const std::string & name, NameLookupImpl *nl) const override;

public:
  Template template_;
};

} // namespace script

#endif // LIBSCRIPT_TEMPLATE_ARG_SCOPE_P_H
