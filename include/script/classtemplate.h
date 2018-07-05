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
class ClassTemplate;
class ClassTemplateImpl;
class PartialTemplateSpecialization;
class PartialTemplateSpecializationImpl;

class ClassBuilder;

typedef Class(*NativeClassTemplateInstantiationFunction)(ClassTemplate, const std::vector<TemplateArgument> &);

class LIBSCRIPT_API ClassTemplate : public Template
{
public:
  ClassTemplate() = default;
  ClassTemplate(const ClassTemplate & other) = default;
  ~ClassTemplate() = default;

  explicit ClassTemplate(const std::shared_ptr<ClassTemplateImpl> & impl);

  bool is_native() const;
  NativeClassTemplateInstantiationFunction native_callback() const;

  bool hasInstance(const std::vector<TemplateArgument> & args, Class *value = nullptr) const;
  Class getInstance(const std::vector<TemplateArgument> & args);

  Class addSpecialization(const std::vector<TemplateArgument> & args, const ClassBuilder & opts);

  Class build(const ClassBuilder & builder, const std::vector<TemplateArgument> & args) const;

  const std::vector<PartialTemplateSpecialization> & partialSpecializations() const;

  const std::map<std::vector<TemplateArgument>, Class, TemplateArgumentComparison> & instances() const;

  std::shared_ptr<ClassTemplateImpl> impl() const;

  ClassTemplate & operator=(const ClassTemplate & other) = default;
};

class LIBSCRIPT_API PartialTemplateSpecialization : public Template
{
public:
  PartialTemplateSpecialization() = default;
  PartialTemplateSpecialization(const PartialTemplateSpecialization & ) = default;
  ~PartialTemplateSpecialization() = default;

  PartialTemplateSpecialization(const std::shared_ptr<PartialTemplateSpecializationImpl> & impl);

  const std::vector<std::shared_ptr<ast::Node>> & arguments() const;
  ClassTemplate specializationOf() const;

  std::shared_ptr<PartialTemplateSpecializationImpl> impl() const;

  PartialTemplateSpecialization & operator=(const PartialTemplateSpecialization & other) = default;
};

} // namespace script

#endif // LIBSCRIPT_CLASS_TEMPLATE_H
