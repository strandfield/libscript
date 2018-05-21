// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/private/templateargumentscope_p.h"

#include "script/engine.h"
#include "script/template.h"
#include "namelookup_p.h"

namespace script
{

TemplateArgumentScope::TemplateArgumentScope(const Template & t, const std::vector<TemplateArgument> & args)
  : template_(t)
  , arguments_(args)
{

}

TemplateArgumentScope::TemplateArgumentScope(const TemplateArgumentScope & other)
  : ScopeImpl(other)
  , template_(other.template_)
  , arguments_(other.arguments_)
{

}

Engine * TemplateArgumentScope::engine() const
{
  return this->parent->engine();
}

int TemplateArgumentScope::kind() const
{
  return Scope::TemplateArgumentScope;
}

TemplateArgumentScope * TemplateArgumentScope::clone() const
{
  return new TemplateArgumentScope(*this);
}

bool TemplateArgumentScope::lookup(const std::string & name, NameLookupImpl *nl) const
{
  const auto & params = template_.parameters();
  size_t size = std::min(params.size(), arguments_.size());
  for (size_t i(0); i < params.size(); ++i)
  {
    if (params.at(i).name() != name)
      continue;

    if (arguments_.size() <= i)
    {
      nl->templateParameterIndex = i;
      return true;
    }

    const auto & targ = arguments_.at(i);
    if (targ.kind == TemplateArgument::TypeArgument)
      nl->typeResult = targ.type;
    else if (targ.kind == TemplateArgument::BoolArgument)
    {
      Value val = engine()->newBool(targ.boolean);
      engine()->manage(val);
      nl->valueResult = val;
    }
    else if (targ.kind == TemplateArgument::IntegerArgument)
    {
      Value val = engine()->newInt(targ.integer);
      engine()->manage(val);
      nl->valueResult = val;
    }
    else if (targ.kind == TemplateArgument::PackArgument)
      throw std::runtime_error{ "Parameter packs not implemented yet" };

    return true;
  }

  return false;
}

void TemplateArgumentScope::add_cast(const Cast & c)
{
  parent->add_cast(c);
}

void TemplateArgumentScope::add_class(const Class & c)
{
  parent->add_class(c);
}

void TemplateArgumentScope::add_function(const Function & f)
{
  parent->add_function(f);
}

void TemplateArgumentScope::add_operator(const Operator & op)
{
  parent->add_operator(op);
}

void TemplateArgumentScope::add_literal_operator(const LiteralOperator & lo)
{
  parent->add_literal_operator(lo);
}

void TemplateArgumentScope::add_enum(const Enum & e)
{
  parent->add_enum(e);
}

void TemplateArgumentScope::add_template(const Template & t)
{
  parent->add_template(t);
}

void TemplateArgumentScope::add_typedef(const Typedef & td)
{
  parent->add_typedef(td);
}

void TemplateArgumentScope::remove_class(const Class & c)
{
  parent->remove_class(c);
}

void TemplateArgumentScope::remove_enum(const Enum & e)
{
  parent->remove_enum(e);
}

void TemplateArgumentScope::remove_function(const Function & f)
{
  parent->remove_function(f);
}

void TemplateArgumentScope::remove_operator(const Operator & op)
{
  parent->remove_operator(op);
}

void TemplateArgumentScope::remove_cast(const Cast & c)
{
  parent->remove_cast(c);
}


TemplateParameterScope::TemplateParameterScope(const Template & t)
  : template_(t)
{

}

TemplateParameterScope::TemplateParameterScope(const TemplateParameterScope & other)
  : ScopeImpl(other)
  , template_(other.template_)
{

}

Engine * TemplateParameterScope::engine() const
{
  return parent->engine();
}

int TemplateParameterScope::kind() const
{
  return Scope::TemplateArgumentScope;
}

TemplateParameterScope * TemplateParameterScope::clone() const
{
  return new TemplateParameterScope(*this);
}

bool TemplateParameterScope::lookup(const std::string & name, NameLookupImpl *nl) const
{
  const auto & params = template_.parameters();
  for (size_t i(0); i < params.size(); ++i)
  {
    if (params.at(i).name() != name)
      continue;

    nl->templateParameterIndex = i;
    return true;
  }

  return false;
}

} // namespace script
