// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_CLASS_TEMPLATE_H
#define LIBSCRIPT_CLASS_TEMPLATE_H

#include "script/template.h"

#include <map>

namespace script
{

class Class;
class ClassTemplateImpl;
class ClassTemplateNativeBackend;
class ClassTemplateSpecializationBuilder;
class PartialTemplateSpecialization;
class PartialTemplateSpecializationImpl;

class LIBSCRIPT_API ClassTemplate : public Template
{
public:
  ClassTemplate() = default;
  ClassTemplate(const ClassTemplate & other) = default;
  ~ClassTemplate() = default;

  explicit ClassTemplate(const std::shared_ptr<ClassTemplateImpl> & impl);

  ClassTemplateNativeBackend* backend() const;

  bool hasInstance(const std::vector<TemplateArgument> & args, Class *value = nullptr) const;
  Class getInstance(const std::vector<TemplateArgument> & args);

  ClassTemplateSpecializationBuilder Specialization(const std::vector<TemplateArgument> & args);
  ClassTemplateSpecializationBuilder Specialization(std::vector<TemplateArgument> && args);

  const std::vector<PartialTemplateSpecialization> & partialSpecializations() const;

  const std::map<std::vector<TemplateArgument>, Class, TemplateArgumentComparison> & instances() const;

  using Template::get;

  template<typename T>
  static ClassTemplate get(Engine* e);

  std::shared_ptr<ClassTemplateImpl> impl() const;

  ClassTemplate & operator=(const ClassTemplate & other) = default;
};

/// TODO: try move outside of this file
class LIBSCRIPT_API PartialTemplateSpecialization
{
public:
  PartialTemplateSpecialization() = default;
  PartialTemplateSpecialization(const PartialTemplateSpecialization & ) = default;
  ~PartialTemplateSpecialization() = default;

  PartialTemplateSpecialization(const std::shared_ptr<PartialTemplateSpecializationImpl> & impl);

  bool isNull() const;

  const std::vector<TemplateParameter>& parameters() const;
  
  // @TODO: it would be better to rely on Template::scope(), argumentScope() & parameterScope()
  Scope scope() const;
  Scope argumentScope(const std::vector<TemplateArgument>& args) const;
  Scope parameterScope() const;

  const std::vector<std::shared_ptr<ast::Node>> & arguments() const;
  ClassTemplate specializationOf() const;

  std::shared_ptr<PartialTemplateSpecializationImpl> impl() const;

  PartialTemplateSpecialization & operator=(const PartialTemplateSpecialization & ) = default;

private:
  std::shared_ptr<PartialTemplateSpecializationImpl> d;
};

LIBSCRIPT_API bool operator==(const PartialTemplateSpecialization& lhs, const PartialTemplateSpecialization& rhs);
inline bool operator!=(const PartialTemplateSpecialization& lhs, const PartialTemplateSpecialization& rhs) { return !(lhs == rhs); }

template<typename T>
inline ClassTemplate ClassTemplate::get(Engine* e)
{
  static_assert(std::is_base_of<ClassTemplateNativeBackend, T>::value, "T must be derived from ClassTemplateNativeBackend");

  const auto& map = get_template_map(e);
  auto it = map.find(std::type_index(typeid(T)));

  if (it == map.end())
  {
    return {};
  }
  else
  {
    return (*it).second.asClassTemplate();
  }
}

} // namespace script

#endif // LIBSCRIPT_CLASS_TEMPLATE_H
