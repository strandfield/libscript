// Copyright (C) 2019 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_TEMPLATE_NATIVE_BACKEND_H
#define LIBSCRIPT_FUNCTION_TEMPLATE_NATIVE_BACKEND_H

#include "script/templatenativebackend.h"

#include "script/function.h"
#include "script/userdata.h"

#include <vector>

namespace script
{

class Function;
class FunctionBuilder;
class FunctionTemplate;
class FunctionTemplateImpl;
class TemplateArgument;
class TemplateArgumentDeduction;
class Type;

class LIBSCRIPT_API FunctionTemplateNativeBackend : public TemplateNativeBackend
{
public:
  FunctionTemplateNativeBackend() = default;
  ~FunctionTemplateNativeBackend() = default;

  FunctionTemplate functionTemplate() const;

  virtual void deduce(TemplateArgumentDeduction& deduction, const std::vector<TemplateArgument>& targs, const std::vector<Type>& itypes) = 0;

  virtual void substitute(FunctionBuilder& builder, const std::vector<TemplateArgument>& targs) = 0;

  virtual std::pair<NativeFunctionSignature, std::shared_ptr<UserData>> instantiate(Function& function) = 0;

private:
  friend class FunctionTemplateBuilder;
  std::weak_ptr<FunctionTemplateImpl> m_template;
};

} // namespace script

#endif // !LIBSCRIPT_FUNCTION_TEMPLATE_NATIVE_BACKEND_H
