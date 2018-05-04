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

class ClassBuilder;

typedef Class(*NativeClassTemplateInstantiationFunction)(ClassTemplate, const std::vector<TemplateArgument> &);

class LIBSCRIPT_API ClassTemplate : public Template
{
public:
  ClassTemplate() = default;
  ClassTemplate(const ClassTemplate & other) = default;
  ~ClassTemplate() = default;

  ClassTemplate(const std::shared_ptr<ClassTemplateImpl> & impl);

  bool hasInstance(const std::vector<TemplateArgument> & args, Class *value = nullptr) const;
  Class getInstance(const std::vector<TemplateArgument> & args);

  Class addSpecialization(const std::vector<TemplateArgument> & args, const ClassBuilder & opts);

  Class build(const ClassBuilder & builder, const std::vector<TemplateArgument> & args) const;

  const std::map<std::vector<TemplateArgument>, Class, TemplateArgumentComparison> & instances() const;

  ClassTemplate & operator=(const ClassTemplate & other) = default;

protected:
  std::shared_ptr<ClassTemplateImpl> impl() const;
};

} // namespace script

#endif // LIBSCRIPT_CLASS_TEMPLATE_H
