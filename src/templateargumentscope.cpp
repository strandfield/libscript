// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/private/templateargumentscope_p.h"

#include "script/engine.h"
#include "script/template.h"
#include "script/private/namelookup_p.h"

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

void TemplateArgumentScope::remove_class(const Class & c)
{
  parent->remove_class(c);
}

void TemplateArgumentScope::remove_enum(const Enum & e)
{
  parent->remove_enum(e);
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
