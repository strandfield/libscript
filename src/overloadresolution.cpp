// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/overloadresolution.h"

#include "script/engine.h"

#include "script/program/expression.h"

namespace script
{

struct OverloadResolutionImpl
{
  Engine *engine;
  std::vector<OverloadResolution::Candidate> candidates;
  int options;
  OverloadResolution::Candidate selected;
  std::vector<ConversionSequence> selected_conversions;
  OverloadResolution::Candidate ambiguous;
  std::vector<ConversionSequence> alternative_conversions;

  OverloadResolutionImpl(Engine *e, int opts)
    : engine(e)
    , options(opts)
  {

  }
};

struct StoreCandidate
{
  OverloadResolutionImpl *data;
  OverloadResolution::Candidate & candidate;

  StoreCandidate(OverloadResolutionImpl *d, OverloadResolution::Candidate & c)
    : data(d), candidate(c) { }

  ~StoreCandidate()
  {
    if (data->options & OverloadResolution::StoreCandidates)
      data->candidates.push_back(candidate);
    data = nullptr;
  }
};

OverloadResolution::Candidate::Candidate()
  : vastatus(OverloadResolution::IncorrectParameterCount)
{

}

OverloadResolution::Candidate::Candidate(const Function & f, ViabilityStatus status)
  : function(f)
  , vastatus(status)
{

}


bool OverloadResolution::Candidate::isViable() const
{
  return viabilityStatus() == Viable;
}

OverloadResolution::ViabilityStatus OverloadResolution::Candidate::viabilityStatus() const
{
  if (vastatus == -2)
    return Viable;
  else if (vastatus == -1)
    return IncorrectParameterCount;
  return CouldNotConvertArgument;
}

int OverloadResolution::Candidate::argument() const
{
  return vastatus;
}


OverloadResolution::Arguments::Arguments(const std::vector<Type> *types)
  : mTypes(types)
  , mValues(nullptr)
  , mExpressions(nullptr)
{

}

OverloadResolution::Arguments::Arguments(const std::vector<Value> *values)
  : mTypes(nullptr)
  , mValues(values)
  , mExpressions(nullptr)
{

}

OverloadResolution::Arguments::Arguments(const std::vector<std::shared_ptr<program::Expression>> *exprs)
  : mTypes(nullptr)
  , mValues(nullptr)
  , mExpressions(exprs)
{

}

int OverloadResolution::Arguments::size() const
{
  if (mTypes != nullptr)
    return mTypes->size();
  else if (mValues != nullptr)
    return mValues->size();
  return mExpressions->size();
}

ConversionSequence OverloadResolution::Arguments::conversion(int argIndex, const Type & dest, Engine *engine) const
{
  if (mTypes != nullptr)
    return ConversionSequence::compute(mTypes->at(argIndex), dest, engine);
  else if (mValues != nullptr)
    return ConversionSequence::compute(mValues->at(argIndex).type(), dest, engine);
  return ConversionSequence::compute(mExpressions->at(argIndex), dest, engine);
}

OverloadResolution::OverloadResolution()
  : d(nullptr)
{

}


void OverloadResolution::setOption(Option opt, bool on)
{
  if (on)
    d->options |= static_cast<int>(opt);
  else
    d->options &= ~static_cast<int>(opt);
}

bool OverloadResolution::testOption(Option opt) const
{
  return d->options & static_cast<int>(opt);
}

int OverloadResolution::options() const
{
  return d->options;
}

bool OverloadResolution::success() const
{
  return !d->selected.function.isNull() && d->ambiguous.function.isNull();
}

Function OverloadResolution::selectedOverload() const
{
  return d->selected.function;
}

const std::vector<ConversionSequence> & OverloadResolution::conversionSequence() const
{
  return d->selected_conversions;
}


Function OverloadResolution::ambiguousOverload() const
{
  return d->ambiguous.function;
}

const std::vector<OverloadResolution::Candidate> & OverloadResolution::candidates() const
{
  return d->candidates;
}

bool OverloadResolution::process(const std::vector<Function> & candidates, const Arguments & arguments)
{
  const int argc = arguments.size();

  std::vector<ConversionSequence> conversions;

  d->candidates.clear();
  for (const auto & func : candidates)
  {
    conversions.clear();
    Candidate candidate{ func, Viable };
    StoreCandidate candidate_storage{ d.get(), candidate };

    if (!func.accepts(argc))
    {
      candidate.vastatus = IncorrectParameterCount;
      continue;
    }

    for (int i(0); i < argc; ++i)
    {
      ConversionSequence conv = arguments.conversion(i, func.parameter(i), d->engine);
      conversions.push_back(conv);
      if (conv == ConversionSequence::NotConvertible())
      {
        candidate.vastatus = CouldNotConvertArgument;
        candidate.mArgument = i;
        break;
      }
    }

    if (candidate.vastatus == Viable)
      processCandidate(candidate, conversions);
  }

  if (d->ambiguous.function.isNull() && !d->selected.function.isNull())
    return true;
  return false;
}

bool OverloadResolution::process(const std::vector<Function> & candidates, const std::vector<Type> & types)
{
  return process(candidates, Arguments{ &types });
}

bool OverloadResolution::process(const std::vector<Function> & candidates, const std::vector<std::shared_ptr<program::Expression>> & arguments)
{
  return process(candidates, Arguments{ &arguments });
}

bool OverloadResolution::process(const std::vector<Function> & candidates, const std::vector<std::shared_ptr<program::Expression>> & arguments, const std::shared_ptr<program::Expression> & object)
{
  if (object == nullptr)
    return process(candidates, arguments);

  const int argc = arguments.size();

  std::vector<ConversionSequence> conversions;

  d->candidates.clear();
  for (const auto & func : candidates)
  {
    conversions.clear();
    Candidate candidate{ func, Viable };
    StoreCandidate candidate_storage{ d.get(), candidate };

    const int actual_argc = func.isMemberFunction() ? argc + 1 : argc;

    if (!func.accepts(actual_argc))
    {
      candidate.vastatus = IncorrectParameterCount;
      continue;
    }

    if (func.isMemberFunction())
    {
      ConversionSequence conv = ConversionSequence::compute(object->type(), func.parameter(0), d->engine);
      conversions.push_back(conv);
      if (conv == ConversionSequence::NotConvertible() || conv.conv1.isCopyInitialization())
      {
        candidate.vastatus = CouldNotConvertArgument;
        candidate.mArgument = 0;
        continue;
      }
    }

    const int parameter_offset = func.isMemberFunction() ? 1 : 0;

    for (int i(0); i < argc; ++i)
    {
      ConversionSequence conv = ConversionSequence::compute(arguments.at(i), func.parameter(i+ parameter_offset), d->engine);
      conversions.push_back(conv);
      if (conv == ConversionSequence::NotConvertible())
      {
        candidate.vastatus = CouldNotConvertArgument;
        candidate.mArgument = i + parameter_offset;
        break;
      }
    }

    if (candidate.vastatus == Viable)
      processCandidate(candidate, conversions);
  }

  if (d->ambiguous.function.isNull() && !d->selected.function.isNull())
    return true;
  return false;
}

bool OverloadResolution::process(const std::vector<Operator> & candidates, const Type & lhs, const Type & rhs)
{
  std::vector<ConversionSequence> conversions;

  d->candidates.clear();
  for (const auto & op : candidates)
  {
    conversions.clear();
    conversions.resize(2);

    Candidate candidate{ op, Viable };
    StoreCandidate candidate_storage{ d.get(), candidate };

    if (!op.isBinary())
    {
      candidate.vastatus = IncorrectParameterCount;
      continue;
    }

    conversions[0] = ConversionSequence::compute(lhs, op.firstOperand(), op.engine());
    if (conversions[0] == ConversionSequence::NotConvertible())
    {
      candidate.vastatus = CouldNotConvertArgument;
      candidate.mArgument = 0;
      continue;
    }

    conversions[1] = ConversionSequence::compute(rhs, op.secondOperand(), op.engine());
    if (conversions[1] == ConversionSequence::NotConvertible())
    {
      candidate.vastatus = CouldNotConvertArgument;
      candidate.mArgument = 1;
      continue;
    }

    if (candidate.vastatus == Viable)
      processCandidate(candidate, conversions);
  }

  if (d->ambiguous.function.isNull() && !d->selected.function.isNull())
    return true;
  return false;
}

bool OverloadResolution::process(const std::vector<Operator> & candidates, const Type & arg)
{
  std::vector<ConversionSequence> conversions;

  d->candidates.clear();
  for (const auto & op : candidates)
  {
    conversions.clear();
    conversions.resize(1);

    Candidate candidate{ op, Viable };
    StoreCandidate candidate_storage{ d.get(), candidate };

    if (op.isBinary())
    {
      candidate.vastatus = IncorrectParameterCount;
      continue;
    }

    conversions[0] = ConversionSequence::compute(arg, op.firstOperand(), op.engine());
    if (conversions[0] == ConversionSequence::NotConvertible())
    {
      candidate.vastatus = CouldNotConvertArgument;
      candidate.mArgument = 0;
      continue;
    }

    if (candidate.vastatus == Viable)
      processCandidate(candidate, conversions);
  }

  if (d->ambiguous.function.isNull() && !d->selected.function.isNull())
    return true;
  return false;
}

void OverloadResolution::processCandidate(const Candidate & c, std::vector<ConversionSequence> & conversions)
{
  if (c.function == d->selected.function || c.function == d->ambiguous.function)
    return;

  OverloadComparison comp = compare(c, conversions, d->selected, d->selected_conversions);
  if (comp == Indistinguishable || comp == NotComparable)
  {
    d->ambiguous = c;
    std::swap(d->alternative_conversions, conversions);
  }
  else if (comp == FirstIsBetter)
  {
    d->selected = c;
    std::swap(d->selected_conversions, conversions);
    
    if (!d->ambiguous.function.isNull())
    {
      comp = compare(d->selected, d->selected_conversions, d->ambiguous, d->alternative_conversions);
      if (comp == FirstIsBetter)
        d->ambiguous = Candidate{};
    }
  }
  else if (comp == SecondIsBetter)
  {
    if (!d->ambiguous.function.isNull())
    {
      comp = compare(c, conversions, d->ambiguous, d->alternative_conversions);
      if (comp == FirstIsBetter) 
      {
        d->ambiguous = c;
        std::swap(d->alternative_conversions, conversions);
      }
    }
  }
}

OverloadResolution::OverloadComparison OverloadResolution::compare(const Candidate & a, const std::vector<ConversionSequence> & conv_a, const Candidate & b, const std::vector<ConversionSequence> & conv_b)
{
  if (b.function.isNull() && !a.function.isNull())
    return FirstIsBetter;
  else if (a.function.isNull() && !b.function.isNull())
    return SecondIsBetter;
  
  if (a.function.isNull() && b.function.isNull())
    return Indistinguishable;

  assert(!a.function.isNull() && !b.function.isNull());

  if (conv_a.size() != conv_b.size())
    throw std::runtime_error{ "Cannot compare candidates with different argument counts" };

  const ConversionSequence::Rank rank_a = ConversionSequence::globalRank(conv_a);
  const ConversionSequence::Rank rank_b = ConversionSequence::globalRank(conv_b);
  if (rank_a < rank_b)
    return FirstIsBetter;
  else if (rank_a > rank_b)
    return SecondIsBetter;

  const int nb_conversions = conv_a.size();
  int first_diff = 0;
  int first_diff_index = -1;
  for (int i(0); i < nb_conversions; ++i)
  {
    first_diff = ConversionSequence::comp(conv_a.at(i), conv_b.at(i));
    if (first_diff)
    {
      first_diff_index = i;
      break;
    }
  }

  if (first_diff_index == -1)
    return Indistinguishable;

  for (int i(first_diff_index+1); i < nb_conversions; ++i)
  {
    const int diff = ConversionSequence::comp(conv_a.at(i), conv_b.at(i));
    if (diff == -first_diff)
      return NotComparable;
  }

  return first_diff == -1 ? FirstIsBetter : SecondIsBetter;
}


OverloadResolution OverloadResolution::New(Engine *engine, int options)
{
  OverloadResolution ret;
  ret.d = std::make_shared<OverloadResolutionImpl>(engine, options);
  return ret;
}


Function OverloadResolution::select(const std::vector<Function> & candidates, const Arguments & arguments)
{
  if (candidates.empty())
    return Function{};

  OverloadResolution resol = OverloadResolution::New(candidates.front().engine(), NoOptions);
  if (resol.process(candidates, arguments))
    return resol.selectedOverload();
  return Function{};
}

Operator OverloadResolution::select(const std::vector<Operator> & candidates, const Type & lhs, const Type & rhs)
{
  if (candidates.empty())
    return Operator{};

  OverloadResolution resol = OverloadResolution::New(candidates.front().engine(), NoOptions);
  if (resol.process(candidates, lhs, rhs))
    return resol.selectedOverload().toOperator();
  return Operator{};
}

} // namespace script