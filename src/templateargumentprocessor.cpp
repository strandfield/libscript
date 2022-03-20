// Copyright (C) 2018-2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/templateargumentprocessor.h"

#include "script/class.h"
#include "script/classtemplate.h"
#include "script/classtemplateinstancebuilder.h"
#include "script/diagnosticmessage.h"
#include "script/private/template_p.h"

#include "script/compiler/compilererrors.h"
#include "script/compiler/literalprocessor.h"
#include "script/compiler/nameresolver.h"
#include "script/compiler/typeresolver.h"

#include "script/ast/node.h"

#include "script/compiler/compiler.h"

namespace script
{

TemplateArgument TemplateArgumentProcessor::argument(const Scope & scp, const std::shared_ptr<ast::Node> & arg)
{
  if (arg->is<ast::Identifier>())
  {
    auto name = std::static_pointer_cast<ast::Identifier>(arg);
    NameLookup lookup = NameLookup::resolve(name, scp);

    if (lookup.resultType() == NameLookup::TypeName)
      return TemplateArgument{ lookup.typeResult() };
    else
      throw compiler::CompilationFailure{ CompilerError::InvalidTemplateArgument };
  }
  else if (arg->is<ast::Literal>())
  {
    const ast::Literal & l = arg->as<ast::Literal>();
    if (l.is<ast::BoolLiteral>())
      return TemplateArgument{ l.token == parser::Token::True };
    else if (l.is<ast::IntegerLiteral>())
      return TemplateArgument{ compiler::LiteralProcessor::generate(std::static_pointer_cast<ast::IntegerLiteral>(arg)) };
    else
      throw compiler::CompilationFailure{ CompilerError::InvalidLiteralTemplateArgument };
  }
  else if (arg->is<ast::TypeNode>())
  {
    auto type = std::static_pointer_cast<ast::TypeNode>(arg);
    return TemplateArgument{ script::compiler::resolve_type(type->value, scp) };
  }

  throw compiler::CompilationFailure{ CompilerError::InvalidTemplateArgument };
}

std::vector<TemplateArgument> TemplateArgumentProcessor::arguments(const Scope & scp, const std::vector<std::shared_ptr<ast::Node>> & args)
{
  std::vector<TemplateArgument> result;
  result.reserve(args.size());
  for (const auto & a : args)
    result.push_back(argument(scp, a));
  return result;
}

Class TemplateArgumentProcessor::instantiate(ClassTemplate & ct, const std::vector<TemplateArgument> & args)
{
  ClassTemplateInstanceBuilder builder{ ct, std::vector<TemplateArgument>{ args} };
  Class ret = ct.backend()->instantiate(builder);
  ct.impl()->instances[args] = ret;
  return ret;
}

Class TemplateArgumentProcessor::process(const Scope & scp, ClassTemplate & ct, const std::shared_ptr<ast::TemplateIdentifier> & tmplt)
{
  try 
  {
    std::vector<TemplateArgument> targs = arguments(scp, tmplt->arguments);
    complete(ct, scp, targs);
    Class c;
    const bool result = ct.hasInstance(targs, &c);
    if (result)
      return c;
    return instantiate(ct, targs);
  }
  catch (const compiler::CompilationFailure& ex)
  {
    // silently discard the error
    (void)ex;
  }
  catch (const TemplateInstantiationError & error)
  {
    // silently discard the error
    (void)error;
  }

  return Class{};
}

void TemplateArgumentProcessor::complete(const Template & t, const Scope &scp, std::vector<TemplateArgument> & args)
{
  if (t.parameters().size() == args.size())
    return;

  for (size_t i(0); i < t.parameters().size(); ++i)
  {
    if (!t.parameters().at(i).hasDefaultValue())
      throw compiler::CompilationFailure{ CompilerError::MissingNonDefaultedTemplateParameter };

    TemplateArgument arg = argument(scp, t.parameters().at(i).defaultValue());
    args.push_back(arg);
  }
}

const std::vector<std::shared_ptr<ast::Node>> & TemplateArgumentProcessor::getTemplateArguments(const std::shared_ptr<ast::Identifier> & tname)
{
  if (tname->is<ast::TemplateIdentifier>())
  {
    const auto & name = tname->as<ast::TemplateIdentifier>();
    return name.arguments;
  }
  else if (tname->is<ast::ScopedIdentifier>())
  {
    return getTemplateArguments(tname->as<ast::ScopedIdentifier>().rhs);
  }

  throw std::runtime_error{ "Bad call to TemplateArgumentProcessor::getTemplateArguments()" };
}

} // namespace script

