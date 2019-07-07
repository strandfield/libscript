// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/templatebuilder.h"

#include "script/classtemplate.h"
#include "script/functiontemplate.h"
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

FunctionTemplate FunctionTemplateBuilder::get()
{
  /// TODO: perform checks
  FunctionTemplate ret{ std::make_shared<FunctionTemplateImpl>(std::move(name), std::move(this->parameters), this->scope,
    std::move(this->backend), symbol.engine(), this->symbol.impl()) };

  ret.backend()->m_template = ret.impl();
  
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

ClassTemplate ClassTemplateBuilder::get()
{
  /// TODO: perform checks
  ClassTemplate ret{ std::make_shared<ClassTemplateImpl>(std::move(this->name), 
    std::move(this->parameters), this->scope, std::move(this->backend), this->symbol.engine(), this->symbol.impl()) };

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
