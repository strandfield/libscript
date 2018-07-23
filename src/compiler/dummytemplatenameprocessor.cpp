// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/dummytemplatenameprocessor.h"

#include "script/class.h"
#include "script/classtemplate.h"

namespace script
{

namespace compiler
{

TemplateArgument DummyTemplateNameProcessor::argument(const Scope & , const std::shared_ptr<ast::Node> & )
{
  throw std::runtime_error{ "Bad call to DummyTemplateNameProcessor::argument()" };
}

Class DummyTemplateNameProcessor::instantiate(ClassTemplate & , const std::vector<TemplateArgument> & )
{
  throw std::runtime_error{ "Bad call to DummyTemplateNameProcessor::instantiate()" };
}

Class DummyTemplateNameProcessor::process(const Scope & , ClassTemplate & , const std::shared_ptr<ast::TemplateIdentifier> &)
{
  return Class{};
}

void DummyTemplateNameProcessor::complete(const Template & , const Scope &, std::vector<TemplateArgument> & )
{
  throw std::runtime_error{ "Bad call to DummyTemplateNameProcessor::complete()" };
}

} // namespace compiler

} // namespace script

