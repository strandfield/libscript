// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TEMPLATE_NAME_PROCESSOR_H
#define LIBSCRIPT_TEMPLATE_NAME_PROCESSOR_H

#include "script/template.h"

#include "script/ast/forwards.h"

namespace script
{

class NameLookup;

namespace compiler
{

class LIBSCRIPT_API TemplateNameProcessor
{
public:
  TemplateNameProcessor() = default;
  TemplateNameProcessor(const TemplateNameProcessor &) = delete;
  virtual ~TemplateNameProcessor() = default;

  virtual TemplateArgument argument(NameLookup &nl, const std::shared_ptr<ast::Node> & arg);
  std::vector<TemplateArgument> arguments(NameLookup &nl, const std::vector<std::shared_ptr<ast::Node>> & args);

  TemplateNameProcessor & operator=(const TemplateNameProcessor &) = delete;
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_TEMPLATE_NAME_PROCESSOR_H
