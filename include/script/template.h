// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TEMPLATE_H
#define LIBSCRIPT_TEMPLATE_H

#include "script/templateargument.h"
#include "script/templateparameter.h"

namespace script
{

class TemplateImpl;

class ClassTemplate;
class Engine;
class FunctionTemplate;
class Scope;
class Script;
class Template;

struct TemplateInstantiationError : public std::runtime_error
{
  using std::runtime_error::runtime_error;
};

class LIBSCRIPT_API Template
{
public:
  Template() = default;
  Template(const Template & other) = default;
  ~Template() = default;

  Template(const std::shared_ptr<TemplateImpl> & impl);

  bool isNull() const;

  Script script() const;
  Engine * engine() const;

  bool isClassTemplate() const;
  bool isFunctionTemplate() const;

  ClassTemplate asClassTemplate() const;
  FunctionTemplate asFunctionTemplate() const;

  const std::string & name() const;
  const std::vector<TemplateParameter> & parameters() const;
  Scope scope() const;
  Scope argumentScope(const std::vector<TemplateArgument> & args) const;
  Scope parameterScope() const;

  TemplateArgument get(const std::string & name, const std::vector<TemplateArgument> & args) const;

  std::weak_ptr<TemplateImpl> weakref() const;
  inline const std::shared_ptr<TemplateImpl> & impl() const { return d; }

  Template & operator=(const Template & other) = default;
  bool operator==(const Template & other) const;
  inline bool operator!=(const Template & other) const { return !operator==(other); }

  inline bool operator<(const Template & other) const { return d < other.d; }

protected:
  std::shared_ptr<TemplateImpl> d;
};

} // namespace script

#endif // LIBSCRIPT_TEMPLATE_H
