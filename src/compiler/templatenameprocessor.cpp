// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/templatenameprocessor.h"

#include "script/compiler/compilererrors.h"
#include "script/compiler/literalprocessor.h"
#include "script/compiler/nameresolver.h"
#include "script/compiler/typeresolver.h"

#include "script/ast/node.h"

#include "script/namelookup.h"

namespace script
{

namespace compiler
{

inline static diagnostic::pos_t dpos(const std::shared_ptr<ast::Node> & node)
{
  const auto & p = node->pos();
  return diagnostic::pos_t{ p.line, p.col };
}

TemplateArgument TemplateNameProcessor::argument(NameLookup &nl, const std::shared_ptr<ast::Node> & arg)
{
  if (arg->is<ast::Identifier>())
  {
    auto name = std::static_pointer_cast<ast::Identifier>(arg);
    NameLookup lookup = NameLookup::resolve(name, nl.scope());
    if (lookup.resultType() == NameLookup::TypeName)
      return TemplateArgument::make(lookup.typeResult());
    else
      throw InvalidTemplateArgument{ dpos(arg) };
  }
  else if (arg->is<ast::Literal>())
  {
    const ast::Literal & l = arg->as<ast::Literal>();
    if (l.is<ast::BoolLiteral>())
      return TemplateArgument::make(l.token == parser::Token::True);
    else if (l.is<ast::IntegerLiteral>())
      return TemplateArgument::make(LiteralProcessor::generate(std::static_pointer_cast<ast::IntegerLiteral>(arg)));
    else
      throw InvalidLiteralTemplateArgument{ dpos(arg) };
  }
  else if (arg->is<ast::TypeNode>())
  {
    auto type = std::static_pointer_cast<ast::TypeNode>(arg);
    TypeResolver<BasicNameResolver> r;
    return TemplateArgument::make(r.resolve(type->value, nl.scope()));
  }

  throw InvalidTemplateArgument{ dpos(arg) };
}

std::vector<TemplateArgument> TemplateNameProcessor::arguments(NameLookup &nl, const std::vector<std::shared_ptr<ast::Node>> & args)
{
  std::vector<TemplateArgument> result;
  result.reserve(args.size());
  for (const auto & a : args)
    result.push_back(argument(nl, a));
  return result;
}

} // namespace compiler

} // namespace script

