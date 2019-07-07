// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TEMPLATE_BUILDER_H
#define LIBSCRIPT_TEMPLATE_BUILDER_H

#include "script/classtemplatenativebackend.h"
#include "script/functiontemplatenativebackend.h"
#include "script/scope.h"
#include "script/symbol.h"
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
  std::unique_ptr<FunctionTemplateNativeBackend> backend;

public:
  FunctionTemplateBuilder() = delete;
  FunctionTemplateBuilder(FunctionTemplateBuilder &&) = default;
  ~FunctionTemplateBuilder() = default;

  FunctionTemplateBuilder(const Symbol & s, const std::string & name);
  FunctionTemplateBuilder(const Symbol & s, std::string && name);

  template<typename T>
  FunctionTemplateBuilder& withBackend()
  {
    backend = std::unique_ptr<FunctionTemplateNativeBackend>(new T());
    return *this;
  }

  FunctionTemplate get();
  void create();
};


class ClassTemplate;

class LIBSCRIPT_API ClassTemplateBuilder : public TemplateBuilder<ClassTemplateBuilder>
{
private:
  typedef TemplateBuilder<ClassTemplateBuilder> Base;

public:
  std::unique_ptr<ClassTemplateNativeBackend> backend;

public:
  ClassTemplateBuilder() = delete;
  ClassTemplateBuilder(ClassTemplateBuilder &&) = default;
  ~ClassTemplateBuilder() = default;

  ClassTemplateBuilder(const Symbol & s, const std::string & name);
  ClassTemplateBuilder(const Symbol & s, std::string && name);

  template<typename T>
  ClassTemplateBuilder& withBackend()
  {
    backend = std::unique_ptr<ClassTemplateNativeBackend>(new T());
    return *this;
  }

  ClassTemplate get();
  void create();
};

} // namespace script

#endif // LIBSCRIPT_TEMPLATE_BUILDER_H
