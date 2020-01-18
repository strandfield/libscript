// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TEMPLATE_H
#define LIBSCRIPT_TEMPLATE_H

#include <map>
#include <stdexcept>
#include <typeindex>
#include <vector>

#include "script/exception.h"
#include "script/templateargument.h"
#include "script/templateparameter.h"

namespace script
{

class TemplateImpl;

class ClassTemplate;
class FunctionTemplate;
class Engine;
class Scope;
class Script;
class Symbol;

class LIBSCRIPT_API TemplateInstantiationError : public Exceptional
{
public:

  enum ErrorCode
  {
    InvalidTemplateArgument = 1,
    InvalidLiteralTemplateArgument,
    MissingNonDefaultedTemplateParameter,
    CompilationFailure,
    InvalidArgumentCount,
    ArgumentMustBeAType,
    ArgumentCannotBeAnEnumeration,
    TypeMustBeDefaultConstructible,
    TypeMustBeCopyConstructible,
    TypeMustBeDestructible,
  };

  explicit TemplateInstantiationError(ErrorCode ec);
  TemplateInstantiationError(ErrorCode ec, std::string mssg);
};

namespace errors
{

const std::error_category& template_instantiation_category() noexcept;

} // namespace errors

class LIBSCRIPT_API Template
{
public:
  Template() = default;
  Template(const Template& other) = default;
  ~Template() = default;

  explicit Template(const std::shared_ptr<TemplateImpl>& impl);

  bool isNull() const;

  Script script() const;
  Engine* engine() const;

  bool isClassTemplate() const;
  bool isFunctionTemplate() const;

  ClassTemplate asClassTemplate() const;
  FunctionTemplate asFunctionTemplate() const;

  const std::string& name() const;
  const std::vector<TemplateParameter>& parameters() const;
  Scope scope() const;
  Scope argumentScope(const std::vector<TemplateArgument>& args) const;
  Scope parameterScope() const;

  TemplateArgument get(const std::string& name, const std::vector<TemplateArgument>& args) const;

  Symbol enclosingSymbol() const;

  std::weak_ptr<TemplateImpl> weakref() const;
  inline const std::shared_ptr<TemplateImpl>& impl() const { return d; }

  Template& operator=(const Template& other) = default;
  bool operator==(const Template& other) const;
  inline bool operator!=(const Template& other) const { return !operator==(other); }

  inline bool operator<(const Template& other) const { return d < other.d; }

protected:
  static const std::map<std::type_index, Template>& get_template_map(Engine* e);

protected:
  std::shared_ptr<TemplateImpl> d;
};

} // namespace script

namespace std
{

template<> struct is_error_code_enum<script::TemplateInstantiationError::ErrorCode> : std::true_type { };

inline std::error_code make_error_code(script::TemplateInstantiationError::ErrorCode e) noexcept
{
  return std::error_code(static_cast<int>(e), script::errors::template_instantiation_category());
}

} // namespace std

#endif // LIBSCRIPT_TEMPLATE_H
