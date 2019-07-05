// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/templatepatternmatching.h"
#include "script/private/templateargumentscope_p.h"

#include "script/class.h"
#include "script/classtemplate.h"
#include "script/engine.h"
#include "script/functiontype.h"
#include "script/namelookup.h"
#include "script/typesystem.h"

#include "script/ast/node.h"

#include "script/compiler/literalprocessor.h"

#include <algorithm>

namespace script
{

TemplatePatternMatching::TemplatePatternMatching(const Template & tmplt, TemplateArgumentDeduction *tad)
  : deductions_(tad)
  , template_(tmplt)
  , scope_(tmplt.parameterScope())
  , working_mode_(PM_TemplateSelection)
{
  arguments_ = &emptyargs_;
}

bool TemplatePatternMatching::match(const std::vector<std::shared_ptr<ast::Node>> & pattern, const std::vector<TemplateArgument> & inputs)
{
  working_mode_ = PM_TemplateSelection;

  size_t s = std::min(inputs.size(), pattern.size());

  for (size_t i(0); i < s; ++i)
  {
    if (!match_(pattern.at(i), inputs.at(i)))
      return false;
  }

  deductions_->agglomerate_deductions();
  return deductions_->success();
}

bool TemplatePatternMatching::match(const std::shared_ptr<ast::FunctionDecl> & pattern, const Prototype & input)
{
  working_mode_ = PM_TemplateSelection;

  match_(pattern->returnType, input.returnType());

  size_t s = std::min((size_t) input.parameterCount(), pattern->params.size());

  for (size_t i(0); i < s; ++i)
  {
    if (!match_(pattern->params.at(i).type, input.at(i)))
      return false;
  }

  deductions_->agglomerate_deductions();
  return deductions_->success();
}

void TemplatePatternMatching::deduce(const std::shared_ptr<ast::FunctionDecl> & pattern, const std::vector<Type> & inputs)
{
  working_mode_ = PM_Deduction;

  deductions_->reset_success_flag();

  /// TODO : parameter packs may be completed by TAD
  if (arguments().size() == getTemplate().parameters().size())
    return;

  size_t nbargs = std::min(inputs.size(), pattern->params.size());

  for (size_t i(0); i < nbargs; ++i)
    deduce(pattern->params.at(i), inputs.at(i));

  deductions_->agglomerate_deductions();
}

void TemplatePatternMatching::deduce(const ast::FunctionParameter & param, const Type & t)
{
  if (param.type.functionType)
  {
    match_(*param.type.functionType, t.baseType());
    return;
  }

  auto pattern = param.type;
  pattern.constQualifier = parser::Token{};
  pattern.reference = parser::Token{};
  match_(pattern, t.baseType());
}

bool TemplatePatternMatching::match_(const ast::QualifiedType & pattern, const Type & input)
{
  if (pattern.constQualifier.isValid() && !input.isConst())
    return false;
  if (pattern.isRef() && !input.isReference())
    return false;
  if (pattern.isRefRef() && !input.isRefRef())
    return false;

  if (pattern.functionType)
    return match_(*pattern.functionType, input);

  if (pattern.type->is<ast::TemplateIdentifier>())
  {
    auto tmpltid = std::static_pointer_cast<ast::TemplateIdentifier>(pattern.type);
    NameLookup lookup = NameLookup::resolve(pattern.type->as<ast::TemplateIdentifier>().getName(), scope_);
    if (lookup.classTemplateResult().isNull())
      return false;

    if (!input.isObjectType())
      return false;

    Class object_type = engine()->typeSystem()->getClass(input);
    if (!object_type.isTemplateInstance())
      return false;

    ClassTemplate arg_template = object_type.instanceOf();

    if (arg_template != lookup.classTemplateResult())
      return false;

    return match_(tmpltid->arguments, object_type.arguments());
  }
  else if (pattern.type->type() == ast::NodeType::SimpleIdentifier)
  {
    NameLookup lookup = NameLookup::resolve(pattern.type->as<ast::SimpleIdentifier>().getName(), scope_);
    if (lookup.templateParameterIndex() != -1)
    {
      Type t = input;

      if (pattern.constQualifier.isValid())
        t = t.withoutConst();
      if (pattern.isRef())
        t = t.withoutFlag(Type::ReferenceFlag);
      if (pattern.isRefRef())
        t = t.withFlag(Type::ForwardReferenceFlag);

      record_deduction(lookup.templateParameterIndex(), TemplateArgument{ t });
      return true;
    }
    else
    {
      return lookup.typeResult() == input;
    }
  }
  else
  {
    auto qualid = std::static_pointer_cast<ast::ScopedIdentifier>(pattern.type);
    return match_(qualid, input);
  }
}

bool TemplatePatternMatching::match_(const ast::FunctionType & pattern, const Type & input)
{
  if (!input.isFunctionType())
    return false;

  FunctionType ft = engine()->typeSystem()->getFunctionType(input);

  if (!match_(pattern.returnType, ft.prototype().returnType()))
    return false;

  if (pattern.params.size() != (size_t)ft.prototype().count())
    return false;

  for (size_t i(0); i < pattern.params.size(); ++i)
  {
    if (!match_(pattern.params.at(i), ft.prototype().at(i)))
      return false;
  }

  return true;
}

bool TemplatePatternMatching::match_(const std::vector<std::shared_ptr<ast::Node>> & pattern, const std::vector<TemplateArgument> & inputs)
{
  // we should have pattern.size() < inputs.size() 
  // because some arguments may be missing in the pattern but not in the instantiated template
  size_t s = std::min(pattern.size(), inputs.size());

  for (size_t i(0); i < s; ++i)
  {
    if (!match_(pattern.at(i), inputs.at(i)))
      return false;
  }

  return true;
}

bool TemplatePatternMatching::match_(const std::shared_ptr<ast::Node> & pattern, const TemplateArgument & input)
{
  if (pattern->is<ast::TypeNode>())
  {
    if (input.kind != TemplateArgument::TypeArgument)
    {
      const ast::QualifiedType qt = pattern->as<ast::TypeNode>().value;
      if (qt.isConst() || qt.reference.isValid())
        return false;

      NameLookup lookup;
      if (qt.type->is<ast::SimpleIdentifier>())
        lookup = NameLookup::resolve(qt.type->as<ast::SimpleIdentifier>().getName(), scope_);
      else if (qt.type->is<ast::TemplateIdentifier>())
        lookup = NameLookup::resolve(qt.type->as<ast::TemplateIdentifier>().getName(), scope_);
      else
        throw std::runtime_error{ "Not implemented" }; /// TODO: can this be reached ?

      if (lookup.templateParameterIndex() != -1)
      {
        record_deduction(lookup.templateParameterIndex(), input);
        return true;
      }
      else if (input.kind == TemplateArgument::IntegerArgument)
      {
        if (!lookup.variable().isNull() || lookup.variable().type().baseType() != Type::Int)
          return false;

        return lookup.variable().toInt() == input.integer;
      }
      else if (input.kind == TemplateArgument::BoolArgument)
      {
        if (!lookup.variable().isNull() || lookup.variable().type().baseType() != Type::Boolean)
          return false;

        return lookup.variable().toBool() == input.boolean; // ok
      }
      
      return false;
    }

    const ast::QualifiedType qt = pattern->as<ast::TypeNode>().value;
    return match_(qt, input.type);
  }
  else
  {
    if (pattern->is<ast::IntegerLiteral>())
    {
      if (input.kind != TemplateArgument::IntegerArgument)
        return false;

      const int val = compiler::LiteralProcessor::generate(std::static_pointer_cast<ast::IntegerLiteral>(pattern));
      return val == input.integer; // ok
    }
    else if (pattern->is<ast::BoolLiteral>())
    {
      if (input.kind != TemplateArgument::BoolArgument)
        return false;

      return input.boolean == (pattern->as<ast::BoolLiteral>().token == parser::Token::Bool);
    }

    /// TODO : pattern is an expr, we may need to evaluate it
    // e.g. template<int N> class foo<N, N+N> { };
    // problem is we don't have the necessary tools for now
    // we would need a scope that supports a combination of template arg / template param / deductions 
    // and a constant expression evaluator
    return false;
  }
}

bool TemplatePatternMatching::match_(const std::shared_ptr<ast::ScopedIdentifier> & pattern, const Type & input)
{
  if (pattern->rhs->is<ast::TemplateIdentifier>())
  {
    if (!input.isObjectType())
      return false;

    Class object_type = engine()->typeSystem()->getClass(input);
    if (!object_type.isTemplateInstance())
      return false;

    ClassTemplate arg_template = object_type.instanceOf();


    auto template_name = ast::SimpleIdentifier::New(pattern->rhs->as<ast::TemplateIdentifier>().name, pattern->rhs->as<ast::TemplateIdentifier>().ast.lock());
    auto qualid = ast::ScopedIdentifier::New(pattern->lhs, pattern->scopeResolution, template_name);
    NameLookup lookup = NameLookup::resolve(qualid, scope_);
    if (lookup.classTemplateResult().isNull())
      return false;

    if (arg_template != lookup.classTemplateResult())
      return false;

    auto tmpltid = std::static_pointer_cast<ast::TemplateIdentifier>(pattern->rhs);
    return match_(tmpltid->arguments, object_type.arguments());
  }
  else
  {
    /// TODO : some template parameter may already have a value that will not be accessible through scope_
    NameLookup lookup = NameLookup::resolve(pattern, scope_);
    return lookup.typeResult() == input;
  }
}


void TemplatePatternMatching::record_deduction(int param_index, const TemplateArgument & value)
{
  if (param_index < (int)arguments_->size()) // this argument does not need to be deduced
    return;

  deductions_->record_deduction(param_index, value);
}

} // namespace script

