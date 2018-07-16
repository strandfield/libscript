// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TEMPLATE_BUILDER_H
#define LIBSCRIPT_TEMPLATE_BUILDER_H

#include "script/scope.h"
#include "script/symbol.h"
#include "script/templatecallbacks.h"
#include "script/templateparameter.h"

namespace script
{

class FunctionTemplate;

class FunctionTemplateBuilder
{
public:
  Symbol symbol;
  std::string name;
  FunctionTemplateCallbacks callbacks;
  std::vector<TemplateParameter> parameters;
  Scope scope;

public:
  FunctionTemplateBuilder() = delete;
  FunctionTemplateBuilder(const FunctionTemplateBuilder &) = default;
  ~FunctionTemplateBuilder() = default;

  FunctionTemplateBuilder(const Symbol & s, const std::string & name);
  FunctionTemplateBuilder(const Symbol & s, std::string && name);

  FunctionTemplateBuilder & setCallbacks(const FunctionTemplateCallbacks & val);
  FunctionTemplateBuilder & deduce(NativeFunctionTemplateDeductionCallback callback);
  FunctionTemplateBuilder & substitute(NativeFunctionTemplateSubstitutionCallback callback);
  FunctionTemplateBuilder & instantiate(NativeFunctionTemplateInstantiationCallback callback);

  FunctionTemplateBuilder & setScope(const Scope & scp);

  FunctionTemplateBuilder & setParams(std::vector<TemplateParameter> && p);
  inline FunctionTemplateBuilder & params(const TemplateParameter & p) { this->parameters.push_back(p); return (*this); }

  template<typename...Args>
  FunctionTemplateBuilder & params(const TemplateParameter & p, const Args &... rest)
  {
    params(p);
    return params(rest...);
  }

  FunctionTemplate get();
  void create();
};

} // namespace script

#endif // LIBSCRIPT_TEMPLATE_BUILDER_H
