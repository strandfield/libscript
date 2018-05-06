// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_TEMPLATE_H
#define LIBSCRIPT_FUNCTION_TEMPLATE_H

#include "script/template.h"

#include "script/function.h"

#include <map>
#include <utility> // std::pair

namespace script
{

class FunctionTemplate;
class FunctionTemplateImpl;

class FunctionBuilder;
class TemplateArgumentDeduction;
class UserData;

typedef void(*NativeFunctionTemplateDeductionCallback)(TemplateArgumentDeduction &, const FunctionTemplate &, const std::vector<TemplateArgument> &, const std::vector<Type> &);
typedef void(*NativeFunctionTemplateSubstitutionCallback)(FunctionBuilder &, FunctionTemplate, const std::vector<TemplateArgument> &);
typedef std::pair<NativeFunctionSignature, std::shared_ptr<UserData>>(*NativeFunctionTemplateInstantiationCallback)(FunctionTemplate, Function);

namespace program
{
class Expression;
} // namespace program

struct LIBSCRIPT_API FunctionTemplateCallbacks
{
  NativeFunctionTemplateDeductionCallback deduction;
  NativeFunctionTemplateSubstitutionCallback substitution;
  NativeFunctionTemplateInstantiationCallback instantiation;
};

class LIBSCRIPT_API FunctionTemplate : public Template
{
public:
  FunctionTemplate() = default;
  FunctionTemplate(const FunctionTemplate & other) = default;
  ~FunctionTemplate() = default;

  FunctionTemplate(const std::shared_ptr<FunctionTemplateImpl> & impl);

  bool is_native() const;
  const FunctionTemplateCallbacks & native_callbacks() const;

  bool hasInstance(const std::vector<TemplateArgument> & args, Function *value = nullptr) const;
  Function getInstance(const std::vector<TemplateArgument> & args);

  Function addSpecialization(const std::vector<TemplateArgument> & args, const FunctionBuilder & opts);

  const std::map<std::vector<TemplateArgument>, Function, TemplateArgumentComparison> & instances() const;

  std::shared_ptr<FunctionTemplateImpl> impl() const;

  FunctionTemplate & operator=(const FunctionTemplate & other) = default;
};

} // namespace script

#endif // LIBSCRIPT_FUNCTION_TEMPLATE_H
