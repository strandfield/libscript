// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/functiontemplateprocessor.h"

#include "script/engine.h"
#include "script/function.h"
#include "script/private/function_p.h"
#include "script/functionbuilder.h"
#include "script/namelookup.h"
#include "script/private/template_p.h"
#include "script/templateargumentdeduction.h"
#include "script/private/templateargumentscope_p.h"

#include "script/compiler/compiler.h"
#include "script/compiler/compilererrors.h"
#include "script/compiler/functionprocessor.h"
#include "script/compiler/nameresolver.h"

#include <algorithm>

namespace script
{

void FunctionTemplateProcessor::remove_duplicates(std::vector<FunctionTemplate> & list)
{
  std::sort(list.begin(), list.end());
  list.erase(std::unique(list.begin(), list.end()), list.end());
}

void FunctionTemplateProcessor::complete(std::vector<Function> & functions, const std::vector<FunctionTemplate> & fts, const std::vector<TemplateArgument> & args, const std::vector<Type> & types)
{
  const auto&  templates = fts;

  for (size_t i(0); i < templates.size(); ++i)
  {
    Function f = deduce_substitute(templates.at(i), args, types);
    if (!f.isNull())
      functions.push_back(f);
  }
}

diagnostic::DiagnosticMessage FunctionTemplateProcessor::emitDiagnostic() const
{
  throw std::runtime_error{ "Not implemented : FunctionTemplateProcessor::emitDiagnostic()" };
}

void FunctionTemplateProcessor::instantiate(Function & f)
{
  FunctionTemplate ft = f.instanceOf();
  const std::vector<TemplateArgument> & targs = f.arguments();

  auto result = ft.backend()->instantiate(f);
  
  if(result.first)
    f.impl()->set_body(builders::make_body(result.first));

  f.impl()->data = result.second;

  //if (ft.is_native())
  //{
  //  auto result = ft.native_callbacks().instantiation(ft, f);
  //  f.impl()->implementation.callback = result.first;
  //  f.impl()->data = result.second;
  //}
  //else
  //{
  //  Engine *e = ft.engine();
  //  compiler::Compiler* compiler = e->compiler();
  //  auto decl = std::static_pointer_cast<ast::FunctionDecl>(ft.impl()->definition.decl_->declaration);
  //  compiler->instantiate(decl, f, ft.argumentScope(f.arguments()));
  //}

  ft.impl()->instances[targs] = f;

  f.impl()->complete_instantiation();
}

Function FunctionTemplateProcessor::deduce_substitute(const FunctionTemplate & ft, const std::vector<TemplateArgument> & args, const std::vector<Type> & types)
{
  const std::vector<TemplateArgument> *template_args = &args;
  std::vector<TemplateArgument> targs_copy;

  if (ft.parameters().size() > args.size())
  {
    TemplateArgumentDeduction deduction_result;

    ft.backend()->deduce(deduction_result, args, types);

    //if (is_native)
    //  ft.native_callbacks().deduction(deduction_result, ft, args, types);
    //else
    //  deduction_result.fill(ft, args, types, ft.impl()->definition.decl_);

    if (!deduction_result.success())
      return Function{};

    targs_copy.reserve(ft.parameters().size());
    targs_copy.insert(targs_copy.begin(), args.begin(), args.end());
    template_args = &targs_copy;

    for (size_t i(args.size()); i < ft.parameters().size(); ++i)
    {
      if (deduction_result.has_deduction_for(i))
        targs_copy.push_back(deduction_result.deduced_value_for(i));
      else
      {
        // use default value
        if (!ft.parameters().at(i).hasDefaultValue())
          return Function{}; // substitution / deduction failure is not an error !

        Scope scp = ft.argumentScope(targs_copy);
        try {
          TemplateArgumentProcessor tnp;
          TemplateArgument arg = tnp.argument(scp, ft.parameters().at(i).defaultValue());
          targs_copy.push_back(arg);
        }
        catch (const compiler::CompilationFailure&)
        {
          return Function{};
        }
      }
    }
  }

  Function f;
  if (ft.hasInstance(*template_args, &f))
    return f;
  
  FunctionBlueprint blueprint{ Symbol(ft.impl()->enclosing_symbol.lock()), SymbolKind::Function, std::string{} };

  ft.backend()->substitute(blueprint, *template_args);

 /* if (is_native)
  {
    ft.native_callbacks().substitution(builder, ft, *template_args);
  }
  else
  {
    compiler::FunctionProcessor fp;

    auto fundecl = std::static_pointer_cast<ast::FunctionDecl>(ft.impl()->definition.decl_->declaration);

    auto tparamscope = ft.argumentScope(*template_args);
    fp.generic_fill(builder, fundecl, tparamscope);
  }*/

  // We create a Function that is not goind to be added to the namespace or class (yet)
  auto impl = FunctionTemplateInstance::create(ft, *template_args, blueprint);
  return Function{ impl };
}

} // namespace script
