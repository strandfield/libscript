// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TEMPLATE_CALLBACKS_H
#define LIBSCRIPT_TEMPLATE_CALLBACKS_H

#include "script/callbacks.h"

#include <utility> // std::pair
#include <vector>

namespace script
{

class TemplateArgument;
class Type;

class Function;
class FunctionTemplate;
class FunctionTemplateImpl;

class FunctionBuilder;
class TemplateArgumentDeduction;
class UserData;

typedef void(*NativeFunctionTemplateDeductionCallback)(TemplateArgumentDeduction &, const FunctionTemplate &, const std::vector<TemplateArgument> &, const std::vector<Type> &);
typedef void(*NativeFunctionTemplateSubstitutionCallback)(FunctionBuilder &, FunctionTemplate, const std::vector<TemplateArgument> &);
typedef std::pair<NativeFunctionSignature, std::shared_ptr<UserData>>(*NativeFunctionTemplateInstantiationCallback)(FunctionTemplate, Function);

struct LIBSCRIPT_API FunctionTemplateCallbacks
{
  NativeFunctionTemplateDeductionCallback deduction;
  NativeFunctionTemplateSubstitutionCallback substitution;
  NativeFunctionTemplateInstantiationCallback instantiation;
};

class Class;
class ClassTemplate;

typedef Class(*NativeClassTemplateInstantiationFunction)(ClassTemplate, const std::vector<TemplateArgument> &);

} // namespace script

#endif // LIBSCRIPT_TEMPLATE_CALLBACKS_H
