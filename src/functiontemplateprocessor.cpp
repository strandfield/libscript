// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/functiontemplateprocessor.h"

#include "script/function.h"
#include "script/templateargumentdeduction.h"
#include "script/private/templateargumentscope_p.h"
#include "script/compiler/compilererrors.h"

#include <algorithm>

namespace script
{

FunctionTemplateProcessor::FunctionTemplateProcessor(std::vector<FunctionTemplate> & fts, const std::vector<TemplateArgument> & args, const std::vector<Type> & types)
  : templates_(&fts)
  , arguments_(&args)
  , types_(&types)
{
  name_ = &default_name_;
}

void FunctionTemplateProcessor::remove_duplicates(std::vector<FunctionTemplate> & list)
{
  std::sort(list.begin(), list.end());
  list.erase(std::unique(list.begin(), list.end()), list.end());
}

void FunctionTemplateProcessor::complete(std::vector<Function> & functions)
{
  const auto&  templates = *templates_;

  for (size_t i(0); i < templates.size(); ++i)
  {
    Function f = process_one(templates.at(i));
    if (!f.isNull())
      functions.push_back(f);
  }
}

diagnostic::Message FunctionTemplateProcessor::emitDiagnostic() const
{
  throw std::runtime_error{ "Not implemented : FunctionTemplateProcessor::emitDiagnostic()" };
}

Scope FunctionTemplateProcessor::template_argument_scope(const FunctionTemplate & ft, const std::vector<TemplateArgument> & args) const
{
  auto ret = std::make_shared<TemplateArgumentScope>(ft, args);
  ret->parent = ft.scope().impl();
  return Scope{ ret };
}

Function FunctionTemplateProcessor::process_one(const FunctionTemplate & ft)
{
  const auto & targs_orig = *arguments_;
  auto *template_args = arguments_;
  std::vector<TemplateArgument> targs_copy;

  if (ft.parameters().size() > targs_orig.size())
  {
    auto deduction_result = ft.deduce(targs_orig, *types_);
    if (!deduction_result.success())
      return Function{};

    targs_copy.reserve(ft.parameters().size());
    targs_copy.insert(targs_copy.begin(), targs_orig.begin(), targs_orig.end());
    template_args = &targs_copy;

    for (size_t i(targs_orig.size()); i < ft.parameters().size(); ++i)
    {
      if (deduction_result.has_deduction_for(i))
        targs_copy.push_back(deduction_result.deduced_value_for(i));
      else
      {
        // use default value
        if (!ft.parameters().at(i).hasDefaultValue())
          return Function{}; // substitution / deduction failure is not an error !

        Scope scp = template_argument_scope(ft, targs_copy);
        try {
          TemplateArgument arg = name_->argument(scp, ft.parameters().at(i).defaultValue());
          targs_copy.push_back(arg);
        }
        catch (const compiler::InvalidTemplateArgument &)
        {
          return Function{};
        }
        catch (const compiler::InvalidLiteralTemplateArgument &)
        {
          return Function{};
        }
      }
    }
  }

  Function f;
  if (ft.hasInstance(*template_args, &f))
    return f;
  
  return ft.substitute(*template_args);
}

} // namespace script
