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

} // namespace script

