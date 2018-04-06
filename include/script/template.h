// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TEMPLATE_H
#define LIBSCRIPT_TEMPLATE_H

#include <map>

#include "types.h"
#include "class.h"

namespace script
{

class TemplateImpl;

class ClassTemplate;
class FunctionTemplate;
class Template;

class FunctionBuilder;


struct LIBSCRIPT_API TemplateArgument
{
  enum Kind {
    TypeArgument,
    IntegerArgument,
    BoolArgument,
  };

  Kind kind;
  Type type;
  int integer;
  bool boolean;

  static TemplateArgument make(const Type & t);
  static TemplateArgument make(int val);
  static TemplateArgument make(bool val);
};

// implements operator< for template arguments
struct TemplateArgumentComparison
{
  static int compare(const TemplateArgument & a, const TemplateArgument & b);
  bool operator()(const TemplateArgument & a, const TemplateArgument & b) const;
  bool operator()(const std::vector<TemplateArgument> & a, const std::vector<TemplateArgument> & b) const;
};

struct TemplateDeductionError : public  std::runtime_error
{
};

typedef bool(*NativeTemplateDeductionFunction)(std::vector<TemplateArgument> &, const std::vector<Type> &);

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

  bool deduce(std::vector<TemplateArgument> &result, const std::vector<Type> & args);

  std::weak_ptr<TemplateImpl> weakref() const;

  Template & operator=(const Template & other) = default;
  bool operator==(const Template & other) const;
  inline bool operator!=(const Template & other) const { return !operator==(other); }

protected:
  std::shared_ptr<TemplateImpl> d;
};

class FunctionTemplate;
typedef Function(*NativeFunctionTemplateInstantiationFunction)(FunctionTemplate, const std::vector<TemplateArgument> &);
class FunctionTemplateImpl;

class LIBSCRIPT_API FunctionTemplate : public Template
{
public:
  FunctionTemplate() = default;
  FunctionTemplate(const FunctionTemplate & other) = default;
  ~FunctionTemplate() = default;

  FunctionTemplate(const std::shared_ptr<FunctionTemplateImpl> & impl);
  
  bool hasInstance(const std::vector<TemplateArgument> & args, Function *value = nullptr) const;
  Function getInstance(const std::vector<TemplateArgument> & args);

  Function addSpecialization(const std::vector<TemplateArgument> & args, const FunctionBuilder & opts);

  const std::map<std::vector<TemplateArgument>, Function, TemplateArgumentComparison> & instances() const;

  FunctionTemplate & operator=(const FunctionTemplate & other) = default;

protected:
  std::shared_ptr<FunctionTemplateImpl> impl() const;
};

typedef Class(*NativeClassTemplateInstantiationFunction)(ClassTemplate, const std::vector<TemplateArgument> &);
class ClassTemplateImpl;

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

  const std::map<std::vector<TemplateArgument>, Class, TemplateArgumentComparison> & instances() const;

  ClassTemplate & operator=(const ClassTemplate & other) = default;

protected:
  std::shared_ptr<ClassTemplateImpl> impl() const;
};

} // namespace script

#endif // LIBSCRIPT_TEMPLATE_H
