// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TEMPLATE_NAME_PROCESSOR_H
#define LIBSCRIPT_TEMPLATE_NAME_PROCESSOR_H

#include "script/template.h"

#include "script/ast/forwards.h"

namespace script
{

class Class;
class Scope;

namespace compiler
{

class LIBSCRIPT_API TemplateNameProcessor
{
public:
  TemplateNameProcessor() = default;
  TemplateNameProcessor(const TemplateNameProcessor &) = delete;
  virtual ~TemplateNameProcessor() = default;

  enum InstantiationPolicy {
    InstantiateIfNeeded,
    FailIfNotInstantiated,
  };

  virtual TemplateArgument argument(const Scope & scp, const std::shared_ptr<ast::Node> & arg);
  std::vector<TemplateArgument> arguments(const Scope & scp, const std::vector<std::shared_ptr<ast::Node>> & args);

  virtual InstantiationPolicy policy() const;
  virtual Class instantiate(ClassTemplate & ct, const std::vector<TemplateArgument> & args);

  virtual void postprocess(const Template & t, const Scope &scp, std::vector<TemplateArgument> & args);

  TemplateNameProcessor & operator=(const TemplateNameProcessor &) = delete;
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_TEMPLATE_NAME_PROCESSOR_H
