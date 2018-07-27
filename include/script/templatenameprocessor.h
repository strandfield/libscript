// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TEMPLATE_NAME_PROCESSOR_H
#define LIBSCRIPT_TEMPLATE_NAME_PROCESSOR_H

#include "libscriptdefs.h"

#include "script/ast/forwards.h"

#include <memory>
#include <vector>

namespace script
{

class Class;
class ClassTemplate;
class Scope;
class Template;
class TemplateArgument;

class LIBSCRIPT_API TemplateNameProcessor
{
public:
  TemplateNameProcessor() = default;
  TemplateNameProcessor(const TemplateNameProcessor &) = delete;
  virtual ~TemplateNameProcessor() = default;

  virtual Class process(const Scope & scp, ClassTemplate & ct, const std::shared_ptr<ast::TemplateIdentifier> & tmplt);

  static const std::vector<std::shared_ptr<ast::Node>> & getTemplateArguments(const std::shared_ptr<ast::Identifier> & tname);

  TemplateNameProcessor & operator=(const TemplateNameProcessor &) = delete;

  /// TODO: make protected
  virtual Class instantiate(ClassTemplate & ct, const std::vector<TemplateArgument> & args);
  std::vector<TemplateArgument> arguments(const Scope & scp, const std::vector<std::shared_ptr<ast::Node>> & args);
  virtual TemplateArgument argument(const Scope & scp, const std::shared_ptr<ast::Node> & arg);

protected:
  virtual void complete(const Template & t, const Scope &scp, std::vector<TemplateArgument> & args);
};

} // namespace script

#endif // LIBSCRIPT_TEMPLATE_NAME_PROCESSOR_H
