// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/templateargumentdeduction.h"
#include "script/private/templateargumentscope_p.h"

#include "script/templatepatternmatching.h"

#include "script/class.h"
#include "script/classtemplate.h"
#include "script/engine.h"
#include "script/functiontype.h"
#include "script/namelookup.h"

#include "script/ast/node.h"

#include "script/compiler/literalprocessor.h"

#include <algorithm>

namespace script
{

TemplateArgumentDeduction::TemplateArgumentDeduction()
  : success_(true)
{

}

size_t TemplateArgumentDeduction::deduction_index(size_t n) const
{
  return deductions_.at(n).param_index;
}

const TemplateArgument & TemplateArgumentDeduction::deduced_value(size_t n) const
{
  return deductions_.at(n).deduced_value;
}

bool TemplateArgumentDeduction::has_deduction_for(size_t param_index) const
{
  for (const auto & d : deductions_)
  {
    if (d.param_index == param_index)
      return true;
  }

  return false;
}

const TemplateArgument & TemplateArgumentDeduction::deduced_value_for(size_t param_index) const
{
  for (const auto & d : deductions_)
  {
    if (d.param_index == param_index)
      return d.deduced_value;
  }

  throw std::runtime_error{"No deduction for given parameter"};
}

TemplateArgumentDeduction TemplateArgumentDeduction::process(FunctionTemplate ft, const std::vector<TemplateArgument> & args, const std::vector<Type> & types, const std::shared_ptr<ast::TemplateDeclaration> & decl)
{
  assert(decl->declaration->is<ast::FunctionDecl>());

  TemplateArgumentDeduction result;
  result.fill(ft, args, types, decl);
  return result;
}

void TemplateArgumentDeduction::fill(FunctionTemplate ft, const std::vector<TemplateArgument> & args, const std::vector<Type> & types, const std::shared_ptr<ast::TemplateDeclaration> & decl)
{
  TemplatePatternMatching2 pm{ ft, this };
  pm.setArguments(args);
  pm.deduce(std::static_pointer_cast<ast::FunctionDecl>(decl->declaration), types);
}

void TemplateArgumentDeduction::agglomerate_deductions()
{
  auto comparator = [](const deduction::Deduction & a, const deduction::Deduction & b) -> bool {
    return a.param_index < b.param_index;
  };

  std::sort(deductions_.begin(), deductions_.end(), comparator);

  auto read = deductions_.cbegin();
  auto write = deductions_.begin();
  auto end = deductions_.end();
  while (read != end)
  {
    if (!process_next_deduction(read, write, end))
      return fail();
  }

  deductions_.erase(write, end);
}

bool TemplateArgumentDeduction::process_next_deduction(std::vector<deduction::Deduction>::const_iterator & read, std::vector<deduction::Deduction>::iterator & write, std::vector<deduction::Deduction>::const_iterator end)
{
  const size_t param_index = read->param_index;
  const TemplateArgument & value = read->deduced_value;

  *write = *read;
  ++read, ++write;
  while (read != end && read->param_index == param_index)
  {
    if (read->deduced_value != value)
      return false;
    
    ++read;
  }

  //write_deduced_argument(param_index, value);

  return true;
}

void TemplateArgumentDeduction::record_deduction(int param_index, const TemplateArgument & value)
{
  deductions_.push_back(deduction::Deduction{ (size_t)param_index, value });
}

void TemplateArgumentDeduction::fail()
{
  success_ = false;
}

TemplatePatternMatching::TemplatePatternMatching(TemplateArgumentDeduction *tad, const Scope & parameterScope)
  : deductions_(tad)
  , scope_(parameterScope)
{

}

bool TemplatePatternMatching::match(const std::vector<std::shared_ptr<ast::Node>> & pattern, const std::vector<TemplateArgument> & inputs)
{
  result_ = true;

  size_t s = std::min(inputs.size(), pattern.size());

  for (size_t i(0); i < s; ++i)
  {
    match_(pattern.at(i), inputs.at(i));
  }

  if (!result_)
    return false;

  deductions_->agglomerate_deductions();
  result_ = deductions_->success();
  return result_;
}

bool TemplatePatternMatching::match(const std::shared_ptr<ast::FunctionDecl> & pattern, const Prototype & input)
{
  result_ = true;

  match_(pattern->returnType, input.returnType());

  size_t s = std::min((size_t) input.parameterCount(), pattern->params.size());

  for (size_t i(0); i < s; ++i)
  {
    match_(pattern->params.at(i).type, input.at(i));
  }

  if (!result_)
    return false;

  deductions_->agglomerate_deductions();
  result_ = deductions_->success();
  return result_;
}

void TemplatePatternMatching::match_(const ast::QualifiedType & pattern, const Type & input)
{
  if (pattern.constQualifier.isValid() && !input.isConst())
    return fail();
  if (pattern.isRef() && !input.isReference())
    return fail();
  if (pattern.isRefRef() && !input.isRefRef())
    return fail();

  if (pattern.functionType)
    return match_(*pattern.functionType, input);

  if (pattern.type->is<ast::TemplateIdentifier>())
  {
    auto tmpltid = std::static_pointer_cast<ast::TemplateIdentifier>(pattern.type);
    NameLookup lookup = NameLookup::resolve(pattern.type->as<ast::TemplateIdentifier>().getName(), scope_);
    if (lookup.classTemplateResult().isNull())
      return fail();

    if (!input.isObjectType())
      return fail();

    Class object_type = engine()->getClass(input);
    if (!object_type.isTemplateInstance())
      return fail();

    ClassTemplate arg_template = object_type.instanceOf();

    if (arg_template != lookup.classTemplateResult())
      return fail();

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

      return record_deduction(lookup.templateParameterIndex(), TemplateArgument{ t });
    }
    else if (lookup.typeResult() == input)
      return; // ok
    else
      return fail();
  }
  else
  {
    auto qualid = std::static_pointer_cast<ast::ScopedIdentifier>(pattern.type);
    return match_(qualid, input);
  }
}

void TemplatePatternMatching::match_(const ast::FunctionType & pattern, const Type & input)
{
  if (!input.isFunctionType())
    return;

  FunctionType ft = engine()->getFunctionType(input);

  match_(pattern.returnType, ft.prototype().returnType());

  if (pattern.params.size() != (size_t)ft.prototype().count())
    return fail();

  for (size_t i(0); i < pattern.params.size(); ++i)
    match_(pattern.params.at(i), ft.prototype().at(i));
}

void TemplatePatternMatching::match_(const std::vector<std::shared_ptr<ast::Node>> & pattern, const std::vector<TemplateArgument> & inputs)
{
  // we should have pattern.size() < inputs.size() 
  // because some arguments may be missing in the pattern but not in the instantiated template
  size_t s = std::min(pattern.size(), inputs.size());

  for (size_t i(0); i < s; ++i)
  {
    match_(pattern.at(i), inputs.at(i));
  }
}

void TemplatePatternMatching::match_(const std::shared_ptr<ast::Node> & pattern, const TemplateArgument & input)
{
  if (pattern->is<ast::TypeNode>())
  {
    if (input.kind != TemplateArgument::TypeArgument)
    {
      const ast::QualifiedType qt = pattern->as<ast::TypeNode>().value;
      if (qt.isConst() || qt.reference.isValid())
        return;

      NameLookup lookup;
      if (qt.type->is<ast::SimpleIdentifier>())
        lookup = NameLookup::resolve(qt.type->as<ast::SimpleIdentifier>().getName(), scope_);
      else if (qt.type->is<ast::TemplateIdentifier>())
        lookup = NameLookup::resolve(qt.type->as<ast::TemplateIdentifier>().getName(), scope_);
      else
        throw std::runtime_error{ "Not implemented" }; /// TODO: can this be reached ?

      if (lookup.templateParameterIndex() != -1)
        return record_deduction(lookup.templateParameterIndex(), input);
      else if (input.kind == TemplateArgument::IntegerArgument)
      {
        if (!lookup.variable().isNull() || lookup.variable().type().baseType() != Type::Int)
          return fail();

        if (lookup.variable().toInt() != input.integer)
          return fail();

        return; // ok
      }
      else if (input.kind == TemplateArgument::BoolArgument)
      {
        if (!lookup.variable().isNull() || lookup.variable().type().baseType() != Type::Boolean)
          return fail();

        if (lookup.variable().toBool() != input.boolean)
          return fail();

        return; // ok
      }
      else
        return fail();
    }

    const ast::QualifiedType qt = pattern->as<ast::TypeNode>().value;
    return match_(qt, input.type);
  }
  else
  {
    if (pattern->is<ast::IntegerLiteral>())
    {
      if (input.kind != TemplateArgument::IntegerArgument)
        return fail();

      const int val = compiler::LiteralProcessor::generate(std::static_pointer_cast<ast::IntegerLiteral>(pattern));
      if (val != input.integer)
        return fail();
      return; // ok
    }
    else if (pattern->is<ast::BoolLiteral>())
    {
      if(input.kind != TemplateArgument::BoolArgument)
        return fail();
      
      if (input.boolean != (pattern->as<ast::BoolLiteral>().token == parser::Token::Bool))
        return fail();

      return; // ok
    }

    /// TODO : pattern is an expr, we may need to evaluate it
    // e.g. template<int N> class foo<N, N+N> { };
    // problem is we don't have the necessary tools for now
    // we would need a scope that supports a combination of template arg / template param / deductions 
    // and a constant expression evaluator
    return fail();
  }
}

void TemplatePatternMatching::match_(const std::shared_ptr<ast::ScopedIdentifier> & pattern, const Type & input)
{
  if (pattern->rhs->is<ast::TemplateIdentifier>())
  {
    if (!input.isObjectType())
      return fail();

    Class object_type = engine()->getClass(input);
    if (!object_type.isTemplateInstance())
      return fail();

    ClassTemplate arg_template = object_type.instanceOf();


    auto template_name = ast::SimpleIdentifier::New(pattern->rhs->as<ast::TemplateIdentifier>().name, pattern->rhs->as<ast::TemplateIdentifier>().ast.lock());
    auto qualid = ast::ScopedIdentifier::New(pattern->lhs, pattern->scopeResolution, template_name);
    NameLookup lookup = NameLookup::resolve(qualid, scope_);
    if (lookup.classTemplateResult().isNull())
      return fail();

    if (arg_template != lookup.classTemplateResult())
      return fail();

    auto tmpltid = std::static_pointer_cast<ast::TemplateIdentifier>(pattern->rhs);
    return match_(tmpltid->arguments, object_type.arguments());
  }
  else
  {
    /// TODO : some template parameter may already have a value that will not be accessible through scope_
    NameLookup lookup = NameLookup::resolve(pattern, scope_);
    if (lookup.typeResult() == input)
      return; // ok
    else
      return fail();
  }
}


void TemplatePatternMatching::record_deduction(int param_index, const TemplateArgument & value)
{
  deductions_->record_deduction(param_index, value);
}

} // namespace script

