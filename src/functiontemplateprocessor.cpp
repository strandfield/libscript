// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/functiontemplateprocessor.h"

#include "script/engine.h"
#include "script/function.h"
#include "function_p.h"
#include "script/functionbuilder.h"
#include "script/namelookup.h"
#include "template_p.h"
#include "script/templateargumentdeduction.h"
#include "script/private/templateargumentscope_p.h"

#include "script/compiler/functioncompiler.h"
#include "script/compiler/compilererrors.h"
#include "script/compiler/functionprocessor.h"

#include <algorithm>

namespace script
{

class NameResolver
{
public:
  compiler::TemplateNameProcessor *template_;
public:
  NameResolver() : template_(nullptr) {}
  explicit NameResolver(compiler::TemplateNameProcessor *tnp) : template_(tnp) { }
  NameResolver(const NameResolver &) = default;
  ~NameResolver() = default;

  inline NameLookup resolve(const std::shared_ptr<ast::Identifier> & name, const Scope & scp)
  {
    return NameLookup::resolve(name, scp, *template_);
  }

  NameResolver & operator=(const NameResolver &) = default;
};


FunctionTemplateProcessor::FunctionTemplateProcessor()
{
  name_ = &default_name_;
}

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

diagnostic::Message FunctionTemplateProcessor::emitDiagnostic() const
{
  throw std::runtime_error{ "Not implemented : FunctionTemplateProcessor::emitDiagnostic()" };
}

void FunctionTemplateProcessor::instantiate(Function & f)
{
  FunctionTemplate ft = f.instanceOf();
  const std::vector<TemplateArgument> & targs = f.arguments();

  if (ft.is_native())
  {
    auto result = ft.native_callbacks().instantiation(ft, f);
    f.implementation()->implementation.callback = result.first;
    f.implementation()->data = result.second;
  }
  else
  {
    Engine *e = ft.engine();
    compiler::Compiler c{ e };
    compiler::FunctionCompiler compiler{ &c, c.session() };
    compiler::CompileFunctionTask task;
    task.declaration = std::static_pointer_cast<ast::FunctionDecl>(ft.impl()->definition.decl_->declaration);
    task.function = f;
    task.scope = template_argument_scope(ft, f.arguments());
    compiler.compile(task);
  }

  ft.impl()->instances[targs] = f;
}

Scope FunctionTemplateProcessor::template_argument_scope(const FunctionTemplate & ft, const std::vector<TemplateArgument> & args) const
{
  auto ret = std::make_shared<TemplateArgumentScope>(ft, args);
  ret->parent = ft.scope().impl();
  return Scope{ ret };
}

Function FunctionTemplateProcessor::deduce_substitute(const FunctionTemplate & ft, const std::vector<TemplateArgument> & args, const std::vector<Type> & types)
{
  const bool is_native = ft.is_native();
  const std::vector<TemplateArgument> *template_args = &args;
  std::vector<TemplateArgument> targs_copy;

  if (ft.parameters().size() > args.size())
  {
    TemplateArgumentDeduction deduction_result;

    if (is_native)
      ft.native_callbacks().deduction(deduction_result, ft, args, types);
    else
      deduction_result.fill(ft, args, types, ft.scope(), ft.impl()->definition.decl_);

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
  
  FunctionBuilder builder = FunctionBuilder::Function(std::string{}, Prototype{});

  if (is_native)
  {
    ft.native_callbacks().substitution(builder, ft, *template_args);
  }
  else
  {
    NameResolver nr{ name_ };
    using TypeResolver = compiler::TypeResolver<NameResolver>;
    TypeResolver tr;
    tr.name_resolver() = nr;
    using PrototypeResolver = compiler::BasicPrototypeResolver<TypeResolver>;
    PrototypeResolver pr;
    pr.type_ = tr;
    compiler::FunctionProcessor<PrototypeResolver> fp;
    fp.prototype_ = pr;

    auto fundecl = std::static_pointer_cast<ast::FunctionDecl>(ft.impl()->definition.decl_->declaration);

    auto tparamscope = template_argument_scope(ft, *template_args);
    fp.fill(builder, fundecl, tparamscope);
  }

  auto impl = std::make_shared<FunctionTemplateInstance>(ft, *template_args, builder.name, builder.proto, ft.engine(), builder.flags);
  impl->implementation.callback = builder.callback;
  impl->data = builder.data;
  return Function{ impl };
}

} // namespace script
