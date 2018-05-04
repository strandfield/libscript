// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_TEMPLATE_H
#define LIBSCRIPT_FUNCTION_TEMPLATE_H

#include "script/template.h"

#include <map>

namespace script
{

class Function;
class FunctionTemplate;
class FunctionTemplateImpl;

class FunctionBuilder;
class TemplateArgumentDeduction;
class UserData;

typedef TemplateArgumentDeduction(*NativeFunctionTemplateDeductionCallback)(const FunctionTemplate &, const std::vector<TemplateArgument> &, const std::vector<Type> &);
typedef Function(*NativeFunctionTemplateInstantiationCallback)(FunctionTemplate, Function);
typedef Function(*NativeFunctionTemplateSubstitutionCallback)(FunctionTemplate, const std::vector<TemplateArgument> &);

namespace program
{
class Expression;
} // namespace program

class LIBSCRIPT_API FunctionTemplate : public Template
{
public:
  FunctionTemplate() = default;
  FunctionTemplate(const FunctionTemplate & other) = default;
  ~FunctionTemplate() = default;

  FunctionTemplate(const std::shared_ptr<FunctionTemplateImpl> & impl);
  
  TemplateArgumentDeduction deduce(const std::vector<TemplateArgument> & args, const std::vector<Type> & types) const;
  Function substitute(const std::vector<TemplateArgument> & targs) const;
  void instantiate(Function & f);

  Function build(const FunctionBuilder & builder, const std::vector<TemplateArgument> & args);
  static void setInstanceData(Function & f, const std::shared_ptr<UserData> & data);

  bool hasInstance(const std::vector<TemplateArgument> & args, Function *value = nullptr) const;
  Function getInstance(const std::vector<TemplateArgument> & args);

  Function addSpecialization(const std::vector<TemplateArgument> & args, const FunctionBuilder & opts);

  const std::map<std::vector<TemplateArgument>, Function, TemplateArgumentComparison> & instances() const;

  FunctionTemplate & operator=(const FunctionTemplate & other) = default;

protected:
  std::shared_ptr<FunctionTemplateImpl> impl() const;
};

} // namespace script

#endif // LIBSCRIPT_FUNCTION_TEMPLATE_H
