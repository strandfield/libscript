// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/templatebuilder.h"

#include "script/private/class_p.h"
#include "script/private/namespace_p.h"
#include "script/private/template_p.h"

namespace script
{

FunctionTemplateBuilder::FunctionTemplateBuilder(const Symbol & s, const std::string & n)
  : Base(s, n)
{

}

FunctionTemplateBuilder::FunctionTemplateBuilder(const Symbol & s, std::string && n)
  : Base(s, std::move(n))
{

}

FunctionTemplateBuilder & FunctionTemplateBuilder::setCallbacks(const FunctionTemplateCallbacks & val)
{
  callbacks = val;
  return (*this);
}

FunctionTemplateBuilder & FunctionTemplateBuilder::deduce(NativeFunctionTemplateDeductionCallback callback)
{
  callbacks.deduction = callback;
  return (*this);
}

FunctionTemplateBuilder & FunctionTemplateBuilder::substitute(NativeFunctionTemplateSubstitutionCallback callback)
{
  callbacks.substitution = callback;
  return (*this);
}

FunctionTemplateBuilder & FunctionTemplateBuilder::instantiate(NativeFunctionTemplateInstantiationCallback callback)
{
  callbacks.instantiation = callback;
  return (*this);
}

FunctionTemplate FunctionTemplateBuilder::get()
{
  /// TODO: perform checks
  FunctionTemplate ret{ std::make_shared<FunctionTemplateImpl>(std::move(name), std::move(this->parameters), this->scope,
    this->callbacks.deduction, this->callbacks.substitution, this->callbacks.instantiation, symbol.engine(), this->symbol.impl()) };
  
  if (symbol.isClass())
    symbol.toClass().impl()->templates.push_back(ret);
  else if (symbol.isNamespace())
    symbol.toNamespace().impl()->templates.push_back(ret);

  return ret;
}

void FunctionTemplateBuilder::create()
{
  get();
}


ClassTemplateBuilder::ClassTemplateBuilder(const Symbol & s, const std::string & name)
  : Base(s, name)
{

}

ClassTemplateBuilder::ClassTemplateBuilder(const Symbol & s, std::string && name)
  : Base(s, std::move(name))
{

}

ClassTemplateBuilder & ClassTemplateBuilder::setCallback(NativeClassTemplateInstantiationFunction val)
{
  this->callback = val;
  return (*this);
}

ClassTemplate ClassTemplateBuilder::get()
{
  /// TODO: perform checks
  ClassTemplate ret{ std::make_shared<ClassTemplateImpl>(std::move(this->name), 
    std::move(this->parameters), this->scope, this->callback, this->symbol.engine(), this->symbol.impl()) };

  if (symbol.isClass())
    symbol.toClass().impl()->templates.push_back(ret);
  else if (symbol.isNamespace())
    symbol.toNamespace().impl()->templates.push_back(ret);

  return ret;
}

void ClassTemplateBuilder::create()
{
  get();
}

} // namespace script
