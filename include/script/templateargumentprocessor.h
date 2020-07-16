// Copyright (C) 2018-2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TEMPLATE_ARGUMENT_PROCESSOR_H
#define LIBSCRIPT_TEMPLATE_ARGUMENT_PROCESSOR_H

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

class LIBSCRIPT_API TemplateArgumentProcessor
{
public:
  TemplateArgumentProcessor() = default;
  TemplateArgumentProcessor(const TemplateArgumentProcessor &) = delete;
  ~TemplateArgumentProcessor() = default;

  Class process(const Scope & scp, ClassTemplate & ct, const std::shared_ptr<ast::TemplateIdentifier> & tmplt);

  static const std::vector<std::shared_ptr<ast::Node>> & getTemplateArguments(const std::shared_ptr<ast::Identifier> & tname);

  TemplateArgumentProcessor & operator=(const TemplateArgumentProcessor &) = delete;

  /// TODO: make protected
  Class instantiate(ClassTemplate & ct, const std::vector<TemplateArgument> & args);
  static std::vector<TemplateArgument> arguments(const Scope & scp, const std::vector<std::shared_ptr<ast::Node>> & args);
  static TemplateArgument argument(const Scope & scp, const std::shared_ptr<ast::Node> & arg);

protected:
  void complete(const Template & t, const Scope &scp, std::vector<TemplateArgument> & args);
};

} // namespace script

#endif // LIBSCRIPT_TEMPLATE_ARGUMENT_PROCESSOR_H
