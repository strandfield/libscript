// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/templatenameprocessor.h"

#include "script/classtemplate.h"
#include "script/private/template_p.h"

#include "script/compiler/compilererrors.h"
#include "script/compiler/literalprocessor.h"
#include "script/compiler/nameresolver.h"
#include "script/compiler/typeresolver.h"

#include "script/ast/node.h"

#include "script/compiler/scriptcompiler.h"

namespace script
{

namespace compiler
{

TemplateArgument TemplateNameProcessor::argument(const Scope & scp, const std::shared_ptr<ast::Node> & arg)
{
  if (arg->is<ast::Identifier>())
  {
    auto name = std::static_pointer_cast<ast::Identifier>(arg);
    NameLookup lookup = NameLookup::resolve(name, scp);
    if (lookup.resultType() == NameLookup::TypeName)
      return TemplateArgument{ lookup.typeResult() };
    else
      throw InvalidTemplateArgument{ dpos(arg) };
  }
  else if (arg->is<ast::Literal>())
  {
    const ast::Literal & l = arg->as<ast::Literal>();
    if (l.is<ast::BoolLiteral>())
      return TemplateArgument{ l.token == parser::Token::True };
    else if (l.is<ast::IntegerLiteral>())
      return TemplateArgument{ LiteralProcessor::generate(std::static_pointer_cast<ast::IntegerLiteral>(arg)) };
    else
      throw InvalidLiteralTemplateArgument{ dpos(arg) };
  }
  else if (arg->is<ast::TypeNode>())
  {
    auto type = std::static_pointer_cast<ast::TypeNode>(arg);
    TypeResolver<BasicNameResolver> r;
    return TemplateArgument{ r.resolve(type->value, scp) };
  }

  throw InvalidTemplateArgument{ dpos(arg) };
}

std::vector<TemplateArgument> TemplateNameProcessor::arguments(const Scope & scp, const std::vector<std::shared_ptr<ast::Node>> & args)
{
  std::vector<TemplateArgument> result;
  result.reserve(args.size());
  for (const auto & a : args)
    result.push_back(argument(scp, a));
  return result;
}

Class TemplateNameProcessor::instantiate(ClassTemplate & ct, const std::vector<TemplateArgument> & args)
{
  if (ct.is_native())
  {
    auto instantiate = ct.native_callback();
    Class ret = instantiate(ct, args);
    ct.impl()->instances[args] = ret;
    return ret;
  }
  else
  {
    Engine *e = ct.engine();
    compiler::ScriptCompiler compiler{ e };
    Class ret = compiler.compileClassTemplate(ct, args);
    ct.impl()->instances[args] = ret;
    return ret;
  }
}

Class TemplateNameProcessor::process(const Scope & scp, ClassTemplate & ct, const std::shared_ptr<ast::TemplateIdentifier> & tmplt)
{
  std::vector<TemplateArgument> targs = arguments(scp, tmplt->arguments);
  postprocess(ct, scp, targs);
  Class c;
  const bool result = ct.hasInstance(targs, &c);
  if (result)
    return c;
  return instantiate(ct, targs);
}

void TemplateNameProcessor::postprocess(const Template & t, const Scope &scp, std::vector<TemplateArgument> & args)
{
  if (t.parameters().size() == args.size())
    return;

  for (size_t i(0); i < t.parameters().size(); ++i)
  {
    if (!t.parameters().at(i).hasDefaultValue())
      throw MissingNonDefaultedTemplateParameter{};

    TemplateArgument arg = argument(scp, t.parameters().at(i).defaultValue());
    args.push_back(arg);
  }
}

const std::vector<std::shared_ptr<ast::Node>> & TemplateNameProcessor::get_trailing_template_arguments(const std::shared_ptr<ast::Identifier> & tname)
{
  if (tname->is<ast::TemplateIdentifier>())
  {
    const auto & name = tname->as<ast::TemplateIdentifier>();
    return name.arguments;
  }
  else if (tname->is<ast::ScopedIdentifier>())
  {
    return get_trailing_template_arguments(tname->as<ast::ScopedIdentifier>().rhs);
  }

  throw std::runtime_error{ "Bad call to TemplateNameProcessor::get_trailing_template_arguments()" };
}

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

void DummyTemplateNameProcessor::postprocess(const Template & , const Scope &, std::vector<TemplateArgument> & )
{
  throw std::runtime_error{ "Bad call to DummyTemplateNameProcessor::postprocess()" };
}

} // namespace compiler

} // namespace script

