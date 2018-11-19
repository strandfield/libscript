// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/templatespecialization.h"

#include "script/compiler/dummytemplatenameprocessor.h"

#include "script/classtemplate.h"
#include "script/functiontemplate.h"
#include "script/namelookup.h"
#include "script/templateargumentdeduction.h"

#include "script/private/template_p.h"

#include <algorithm> // std::min

namespace script
{

namespace compiler
{

TemplatePartialOrdering operator&(const TemplatePartialOrdering & lhs, const TemplatePartialOrdering & rhs)
{
  static const TemplatePartialOrdering table[4][4] = {
    { TemplatePartialOrdering::NotComparable,TemplatePartialOrdering::NotComparable, TemplatePartialOrdering::NotComparable, TemplatePartialOrdering::NotComparable },
    { TemplatePartialOrdering::NotComparable,TemplatePartialOrdering::Indistinguishable, TemplatePartialOrdering::FirstIsMoreSpecialized, TemplatePartialOrdering::SecondIsMoreSpecialized },
    { TemplatePartialOrdering::NotComparable,TemplatePartialOrdering::FirstIsMoreSpecialized, TemplatePartialOrdering::FirstIsMoreSpecialized, TemplatePartialOrdering::NotComparable },
    { TemplatePartialOrdering::NotComparable,TemplatePartialOrdering::SecondIsMoreSpecialized, TemplatePartialOrdering::NotComparable, TemplatePartialOrdering::SecondIsMoreSpecialized },
  };

  return table[lhs.value_][rhs.value_];
}


TemplatePartialOrdering TemplateSpecialization::compare(const FunctionTemplate & a, const FunctionTemplate & b)
{
  if (a.name() != b.name())
    return TemplatePartialOrdering::NotComparable;

  if (a.is_native() || b.is_native())
    return TemplatePartialOrdering::Indistinguishable;

  const auto & a_params = a.impl()->definition.get_function_decl()->params;
  const auto & b_params = b.impl()->definition.get_function_decl()->params;
  const size_t s = std::min(a_params.size(), b_params.size());
  TemplatePartialOrdering c{ TemplatePartialOrdering::Indistinguishable };
  for (size_t i(0); i < s; ++i)
  {
    c =  c & compare(SType{ a.parameterScope(), a_params.at(i).type }, SType{ b.parameterScope(), b_params.at(i).type });
    if (c == TemplatePartialOrdering::NotComparable)
      return c;
  }

  if (c.positive())
    return c;

  assert(c == TemplatePartialOrdering::Indistinguishable);

  const bool a_const = a.impl()->definition.get_function_decl()->constQualifier.isValid();
  const bool b_const = b.impl()->definition.get_function_decl()->constQualifier.isValid();

  if (a_const && !b_const)
    return TemplatePartialOrdering::FirstIsMoreSpecialized;
  else if (b_const && !a_const)
    return TemplatePartialOrdering::SecondIsMoreSpecialized;

  if (a.parameters().size() < b.parameters().size())
    return TemplatePartialOrdering::FirstIsMoreSpecialized;
  else if(b.parameters().size() < a.parameters().size())
    return TemplatePartialOrdering::SecondIsMoreSpecialized;

  return TemplatePartialOrdering::Indistinguishable;
}

TemplatePartialOrdering TemplateSpecialization::compare(const SVarDecl & a, const SVarDecl & b)
{
  return compare(SType{ a.scope, a.value->variable_type }, SType{ b.scope, b.value->variable_type });
}

TemplatePartialOrdering TemplateSpecialization::compare(const SType & a, const SType & b)
{
  if (a.value.functionType != nullptr && b.value.functionType == nullptr)
    return TemplatePartialOrdering::FirstIsMoreSpecialized;
  else if (a.value.functionType == nullptr && b.value.functionType != nullptr)
    return TemplatePartialOrdering::SecondIsMoreSpecialized;

  if (a.value.functionType != nullptr)
    return compare(SFunctionType{ a.scope, *a.value.functionType }, SFunctionType{ b.scope, *b.value.functionType });

  if (a.value.type->is<ast::TemplateIdentifier>() && !b.value.type->is<ast::TemplateIdentifier>())
    return TemplatePartialOrdering::FirstIsMoreSpecialized;
  else if (!a.value.type->is<ast::TemplateIdentifier>() && b.value.type->is<ast::TemplateIdentifier>())
    return TemplatePartialOrdering::SecondIsMoreSpecialized;

  DummyTemplateNameProcessor tname_;
  NameLookup lookup_a = NameLookup::resolve(a.value.type, a.scope, tname_);
  NameLookup lookup_b = NameLookup::resolve(b.value.type, b.scope, tname_);

  auto rta = lookup_a.resultType();
  auto rtb = lookup_b.resultType();

  if (rta == NameLookup::TypeName && rtb != NameLookup::TypeName)
    return TemplatePartialOrdering::FirstIsMoreSpecialized;
  else if (rta != NameLookup::TypeName && rtb == NameLookup::TypeName)
    return TemplatePartialOrdering::SecondIsMoreSpecialized;
  if (rta == NameLookup::TypeName && rtb == NameLookup::TypeName)
    return compare_from_qual(a.value, b.value);

  if (rta == NameLookup::TemplateName && rtb != NameLookup::TemplateName)
    return TemplatePartialOrdering::FirstIsMoreSpecialized;
  else if (rta != NameLookup::TemplateName && rtb == NameLookup::TemplateName)
    return TemplatePartialOrdering::SecondIsMoreSpecialized;

  if (rta == NameLookup::TemplateName && rtb == NameLookup::TemplateName)
  {
    if (lookup_a.classTemplateResult() != lookup_b.classTemplateResult())
      return TemplatePartialOrdering::NotComparable; // neither is more specialized

    const auto & a_args = TemplateNameProcessor::getTemplateArguments(a.value.type);
    const auto & b_args = TemplateNameProcessor::getTemplateArguments(b.value.type);
    auto ret = compare_from_args(a.scope, a_args, b.scope, b_args);
    if (ret.positive())
      return ret;

    return compare_from_qual(a.value, b.value);
  }

  if (rta == NameLookup::TemplateParameterName && rtb == NameLookup::TemplateParameterName)
    return compare_from_qual(a.value, b.value);

  return TemplatePartialOrdering::NotComparable; // not implemented or not comparable
}

TemplatePartialOrdering TemplateSpecialization::compare_from_args(const Scope & scpa, const std::vector<std::shared_ptr<ast::Node>> & a, const Scope & scpb, const std::vector<std::shared_ptr<ast::Node>> & b)
{
  if (a.size() > b.size())
    return TemplatePartialOrdering::FirstIsMoreSpecialized;
  else if (a.size() < b.size())
    return TemplatePartialOrdering::SecondIsMoreSpecialized;

  TemplatePartialOrdering c{ TemplatePartialOrdering::Indistinguishable };
  for (size_t i(0); i < a.size(); ++i)
  {
    c = c & compare_targ(STemplateArg{ scpa, a.at(i) }, STemplateArg{ scpb, b.at(i) });
    if (c == TemplatePartialOrdering::NotComparable)
      return c;
  }

  return c;
}

TemplatePartialOrdering TemplateSpecialization::compare_targ(const STemplateArg & a, const STemplateArg & b)
{
  if (a.value->is<ast::TypeNode>() != b.value->is<ast::TypeNode>())
    return TemplatePartialOrdering::NotComparable;

  if (!a.value->is<ast::TypeNode>())
    return TemplatePartialOrdering::Indistinguishable;

  return compare(SType{ a.scope, a.value->as<ast::TypeNode>().value }, SType{ b.scope, b.value->as<ast::TypeNode>().value });
}

TemplatePartialOrdering TemplateSpecialization::compare_from_qual(const ast::QualifiedType & a, const ast::QualifiedType & b)
{
  if (a.constQualifier.isValid() && !b.constQualifier.isValid())
  {
    if ((a.reference.isValid() && !b.reference.isValid()) || a.reference.isValid() == b.reference.isValid())
      return TemplatePartialOrdering::FirstIsMoreSpecialized;

    return TemplatePartialOrdering::NotComparable;
  }
  else if (!a.constQualifier.isValid() && b.constQualifier.isValid())
  {
    if ((b.reference.isValid() && !b.reference.isValid()) || a.reference.isValid() == b.reference.isValid())
      return TemplatePartialOrdering::SecondIsMoreSpecialized;

    return TemplatePartialOrdering::NotComparable;
  }

  assert(a.constQualifier.isValid() == b.constQualifier.isValid());

  if (a.reference.isValid() && !b.reference.isValid())
    return TemplatePartialOrdering::FirstIsMoreSpecialized;
  else if (!a.reference.isValid() && b.reference.isValid())
    return TemplatePartialOrdering::SecondIsMoreSpecialized;

  return TemplatePartialOrdering::Indistinguishable;
}

TemplatePartialOrdering TemplateSpecialization::compare(const SFunctionType & a, const SFunctionType & b)
{
  return TemplatePartialOrdering::Indistinguishable; // for now /// TODO : implement comparison of function types
}

TemplatePartialOrdering TemplateSpecialization::compare(const PartialTemplateSpecialization & a, const PartialTemplateSpecialization & b)
{
  if (a.specializationOf() != b.specializationOf())
    return TemplatePartialOrdering::NotComparable;

  return compare_from_args(a.parameterScope(), a.arguments(), b.parameterScope(), b.arguments());
}


std::pair<PartialTemplateSpecialization, std::vector<TemplateArgument>> TemplateSpecializationSelector::select(const ClassTemplate & ct, const std::vector<TemplateArgument> & targs)
{
  const auto & specs = ct.partialSpecializations();

  std::pair<PartialTemplateSpecialization, std::vector<TemplateArgument>> best;
  for (const auto & tps : specs)
  {
    TemplateArgumentDeduction deduc;
    TemplatePatternMatching pattern{ &deduc, tps.parameterScope() };
    if (!pattern.match(tps.arguments(), targs))
      continue;

    std::vector<TemplateArgument> args;
    bool ok = true;
    for (size_t i(0); i < tps.parameters().size(); ++i)
    {
      if (!deduc.has_deduction_for(i))
      {
        ok = false;
        break;
      }
      args.push_back(deduc.deduced_value_for(i));
    }

    if (!ok)
      continue;

    if (best.first.isNull())
      best = std::make_pair(tps, args);
    else
    {
      TemplatePartialOrdering order = TemplateSpecialization::compare(best.first, tps);
      if (order == TemplatePartialOrdering::SecondIsMoreSpecialized)
        best = std::make_pair(tps, args);
      else if (order == TemplatePartialOrdering::Indistinguishable || order == TemplatePartialOrdering::NotComparable)
        return {}; /// TODO : not totally correct but will do for now
    }
  }

  return best;
}

std::pair<FunctionTemplate, std::vector<TemplateArgument>> TemplateOverloadSelector::select(const std::vector<Template> & fts, const std::vector<TemplateArgument> & targs, const Prototype & proto)
{
  std::pair<FunctionTemplate, std::vector<TemplateArgument>> best;
  for (const auto & ft_ : fts)
  {
    FunctionTemplate ft = ft_.asFunctionTemplate();

    TemplateArgumentDeduction deduc;
    TemplatePatternMatching pattern{ &deduc, ft.argumentScope(targs) };
    if (!pattern.match(ft.impl()->definition.get_function_decl(), proto))
      continue;

    std::vector<TemplateArgument> args;
    bool ok = true;
    for (size_t i(0); i < ft.parameters().size(); ++i)
    {
      if (targs.size() > i)
        args.push_back(targs.at(i));
      else if(deduc.has_deduction_for(i))
        args.push_back(deduc.deduced_value_for(i));
      else if (ft.parameters().at(i).hasDefaultValue())
      {
        /// TODO : push evaluation of  ft.parameters().at(i).defaultValue()
        ok = false;
        break;
      }
    }

    if (!ok)
      continue;

    if (best.first.isNull())
      best = std::make_pair(ft, args);
    else
    {
      TemplatePartialOrdering order = TemplateSpecialization::compare(best.first, ft);
      if (order == TemplatePartialOrdering::SecondIsMoreSpecialized)
        best = std::make_pair(ft, args);
      else if (order == TemplatePartialOrdering::Indistinguishable || order == TemplatePartialOrdering::NotComparable)
        return {}; /// TODO : not totally correct but will do for now
    }
  }

  return best;
}

} // namespace compiler

} // namespace script
