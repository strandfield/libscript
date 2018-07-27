// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_DUMMY_TEMPLATE_NAME_PROCESSOR_H
#define LIBSCRIPT_DUMMY_TEMPLATE_NAME_PROCESSOR_H

#include "script/templatenameprocessor.h"

namespace script
{

namespace compiler
{

class LIBSCRIPT_API DummyTemplateNameProcessor : public TemplateNameProcessor
{
public:
  ~DummyTemplateNameProcessor() = default;

  TemplateArgument argument(const Scope & scp, const std::shared_ptr<ast::Node> & arg) override;
  Class instantiate(ClassTemplate & ct, const std::vector<TemplateArgument> & args) override;
  Class process(const Scope & scp, ClassTemplate & ct, const std::shared_ptr<ast::TemplateIdentifier> & tmplt) override;
  void complete(const Template & t, const Scope &scp, std::vector<TemplateArgument> & args) override;
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_DUMMY_TEMPLATE_NAME_PROCESSOR_H
