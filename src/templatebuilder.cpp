// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/templatebuilder.h"

#include "script/classtemplate.h"
#include "script/engine.h"
#include "script/functiontemplate.h"

#include "script/private/class_p.h"
#include "script/private/engine_p.h"
#include "script/private/namespace_p.h"
#include "script/private/template_p.h"

namespace script
{

FunctionTemplateBuilder::FunctionTemplateBuilder(const Symbol & s, std::string n)
  : Base(s, std::move(n))
{

}

FunctionTemplate FunctionTemplateBuilder::get()
{
  /// TODO: perform checks
  const auto ti = std::type_index(typeid(*this->backend.get()));

  FunctionTemplate ret{ std::make_shared<FunctionTemplateImpl>(std::move(name), std::move(this->parameters), this->scope,
    std::move(this->backend), symbol.engine(), this->symbol.impl()) };

  ret.backend()->m_template = ret.impl();
  
  if (symbol.isClass())
    symbol.toClass().impl()->templates.push_back(ret);
  else if (symbol.isNamespace())
    symbol.toNamespace().impl()->templates.push_back(ret);

  if (ti != std::type_index(typeid(ScriptFunctionTemplateBackend)))
  {
    ret.engine()->implementation()->templates.dict[ti] = ret;
  }

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
  const auto ti = std::type_index(typeid(*this->backend.get()));

  /// TODO: perform checks
  ClassTemplate ret{ std::make_shared<ClassTemplateImpl>(std::move(this->name), 
    std::move(this->parameters), this->scope, std::move(this->backend), this->symbol.engine(), this->symbol.impl()) };

  if (symbol.isClass())
    symbol.toClass().impl()->templates.push_back(ret);
  else if (symbol.isNamespace())
    symbol.toNamespace().impl()->templates.push_back(ret);

  if (ti != std::type_index(typeid(ScriptClassTemplateBackend)))
  {
    ret.engine()->implementation()->templates.dict[ti] = ret;
  }

  return ret;
}

void ClassTemplateBuilder::create()
{
  get();
}

} // namespace script
