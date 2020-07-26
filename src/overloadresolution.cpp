// Copyright (C) 2018-2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/overloadresolution.h"

#include "script/diagnosticmessage.h"
#include "script/class.h"
#include "script/conversions.h"
#include "script/engine.h"
#include "script/operator.h"
#include "script/typesystem.h"

#include "script/program/expression.h"

namespace script
{

OverloadResolution::OverloadComparison OverloadResolution::compare(const Candidate& a, const Candidate& b)
{
  if (b.function.isNull() && !a.function.isNull())
    return OverloadResolution::FirstIsBetter;
  else if (a.function.isNull() && !b.function.isNull())
    return OverloadResolution::SecondIsBetter;

  if (a.function.isNull() && b.function.isNull())
    return OverloadResolution::Indistinguishable;

  assert(!a.function.isNull() && !b.function.isNull());
  assert(a.initializations.size() == b.initializations.size());

  const ConversionRank rank_a = ranking::worstRank(a.initializations);
  const ConversionRank rank_b = ranking::worstRank(b.initializations);
  if (rank_a < rank_b)
    return OverloadResolution::FirstIsBetter;
  else if (rank_a > rank_b)
    return OverloadResolution::SecondIsBetter;

  const size_t nb_conversions = a.initializations.size();
  int first_diff = 0;
  int first_diff_index = -1;
  for (size_t i(0); i < nb_conversions; ++i)
  {
    first_diff = Initialization::comp(a.initializations.at(i), b.initializations.at(i));
    if (first_diff)
    {
      first_diff_index = static_cast<int>(i);
      break;
    }
  }

  if (first_diff_index == -1)
    return OverloadResolution::Indistinguishable;

  for (size_t i(static_cast<size_t>(first_diff_index) + 1); i < nb_conversions; ++i)
  {
    const int diff = Initialization::comp(a.initializations.at(i), b.initializations.at(i));
    if (diff == -first_diff)
      return OverloadResolution::NotComparable;
  }

  return first_diff == -1 ? OverloadResolution::FirstIsBetter : OverloadResolution::SecondIsBetter;
}

OverloadResolution::OverloadResolution()
{

}

} // namespace script