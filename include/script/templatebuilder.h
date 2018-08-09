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

template<typename Derived>
class TemplateBuilder
{
public:
  Symbol symbol;
  std::string name;
  std::vector<TemplateParameter> parameters;
  Scope scope;

public:
  TemplateBuilder(const Symbol & s, const std::string & n)
    : symbol(s)
    , name(n)
  {

  }

  TemplateBuilder(const Symbol & s, std::string && n)
    : symbol(s)
    , name(std::move(n))
  {

  }

  Derived & setScope(const Scope & scp)
  {
    this->scope = scp;
    return *(static_cast<Derived*>(this));
  }

  Derived & setParams(std::vector<TemplateParameter> && p)
  {
    this->parameters = std::move(p);
    return *(static_cast<Derived*>(this));
  }

  Derived & params(const TemplateParameter & p) { this->parameters.push_back(p); return *(static_cast<Derived*>(this)); }

  template<typename...Args>
  Derived & params(const TemplateParameter & p, const Args &... rest)
  {
    params(p);
    return params(rest...);
  }
};

class FunctionTemplate;

class LIBSCRIPT_API FunctionTemplateBuilder : public TemplateBuilder<FunctionTemplateBuilder>
{
private:
  typedef TemplateBuilder<FunctionTemplateBuilder> Base;

public:
  FunctionTemplateCallbacks callbacks;

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

  FunctionTemplate get();
  void create();
};


class ClassTemplate;

class LIBSCRIPT_API ClassTemplateBuilder : public TemplateBuilder<ClassTemplateBuilder>
{
private:
  typedef TemplateBuilder<ClassTemplateBuilder> Base;

public:
  NativeClassTemplateInstantiationFunction callback;

public:
  ClassTemplateBuilder() = delete;
  ClassTemplateBuilder(const ClassTemplateBuilder &) = default;
  ~ClassTemplateBuilder() = default;

  ClassTemplateBuilder(const Symbol & s, const std::string & name);
  ClassTemplateBuilder(const Symbol & s, std::string && name);

  ClassTemplateBuilder & setCallback(NativeClassTemplateInstantiationFunction val);

  ClassTemplate get();
  void create();
};

} // namespace script

#endif // LIBSCRIPT_TEMPLATE_BUILDER_H
