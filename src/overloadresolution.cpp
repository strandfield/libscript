// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/overloadresolution.h"

#include "script/diagnosticmessage.h"
#include "script/class.h"
#include "script/conversions.h"
#include "script/engine.h"
#include "script/operator.h"

#include "script/program/expression.h"

namespace script
{

struct OverloadResolutionImpl
{
  Engine *engine;
  int options;
  Function selected;
  std::vector<ConversionSequence> selected_conversions;
  Function ambiguous;
  std::vector<ConversionSequence> alternative_conversions;
  std::vector<Function> const *candidates;
  OverloadResolution::Arguments arguments;
  std::shared_ptr<program::Expression> implicit_object;

  OverloadResolutionImpl(Engine *e, int opts)
    : engine(e)
    , options(opts)
    , candidates(nullptr)
  {

  }
};


OverloadResolution::Arguments::Arguments()
  : mTypes(nullptr)
  , mValues(nullptr)
  , mExpressions(nullptr)
{

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

OverloadResolution::Arguments::Kind OverloadResolution::Arguments::kind() const
{
  if (mTypes != nullptr)
    return TypeArguments;
  else if (mValues != nullptr)
    return ValueArguments;
  else if (mExpressions != nullptr)
    return ExpressionArguments;
  return Null;
}

const std::vector<Type> & OverloadResolution::Arguments::types() const
{
  return *mTypes;
}

const std::vector<Value> & OverloadResolution::Arguments::values() const
{
  return *mValues;
}

const std::vector<std::shared_ptr<program::Expression>> & OverloadResolution::Arguments::expressions() const
{
  return *mExpressions;
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

OverloadResolution::Arguments & OverloadResolution::Arguments::operator=(const Arguments & other)
{
  this->mTypes = other.mTypes;
  this->mValues = other.mValues;
  this->mExpressions = other.mExpressions;
  return (*this);
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
  return !d->selected.isNull() && d->ambiguous.isNull();
}

Function OverloadResolution::selectedOverload() const
{
  return d->selected;
}

const std::vector<ConversionSequence> & OverloadResolution::conversionSequence() const
{
  return d->selected_conversions;
}


Function OverloadResolution::ambiguousOverload() const
{
  return d->ambiguous;
}

const std::vector<Function> & OverloadResolution::candidates() const
{
  return *(d->candidates);
}

OverloadResolution::ViabilityStatus OverloadResolution::getViabilityStatus(const Function & f, std::vector<ConversionSequence> *conversions) const
{
  std::vector<ConversionSequence> convs;
  if (conversions == nullptr)
    conversions = &convs;

  conversions->clear();

  const int implicit_object_offset = (f.hasImplicitObject() && d->implicit_object != nullptr) ? 1 : 0;
  const int argc = d->arguments.size() + implicit_object_offset;

  if (!f.accepts(argc))
    return IncorrectParameterCount;

  if (implicit_object_offset)
  {
    ConversionSequence conv = ConversionSequence::compute(d->implicit_object->type(), f.parameter(0), d->engine);
    conversions->push_back(conv);
    if (conv == ConversionSequence::NotConvertible() || conv.conv1.isCopyInitialization())
      return CouldNotConvertArgument;
  }

  for (int i(0); i < argc; ++i)
  {
    ConversionSequence conv = d->arguments.conversion(i, f.parameter(i + implicit_object_offset), d->engine);
    conversions->push_back(conv);
    if (conv == ConversionSequence::NotConvertible())
      return CouldNotConvertArgument;
  }

  return Viable;
}

OverloadResolution::ViabilityStatus OverloadResolution::getViabilityStatus(int candidate_index, std::vector<ConversionSequence> *conversions) const
{
  return getViabilityStatus(d->candidates->at(candidate_index), conversions);
}

const OverloadResolution::Arguments & OverloadResolution::arguments() const
{
  return d->arguments;
}

const std::shared_ptr<program::Expression> & OverloadResolution::implicit_object() const
{
  return d->implicit_object;
}

diagnostic::Message OverloadResolution::emitDiagnostic() const
{
  if (success())
    return diagnostic::info() << "Overload resolution succeeded";

  if (!d->ambiguous.isNull())
  {
    auto diag = diagnostic::info() << "Overload resolution failed because at least two candidates are not comparable or indistinguishable \n";
    write_prototype(diag, d->selected);
    diag << "\n";
    write_prototype(diag, d->ambiguous);
    return diag;
  }
  
  auto diag = diagnostic::info() << "Overload resolution failed because no viable overload could be found \n";
  std::vector<ConversionSequence> convs;
  for (const auto & f : *(d->candidates))
  {
    auto status = getViabilityStatus(f, &convs);
    write_prototype(diag, f);
    if (status == IncorrectParameterCount)
      diag << "\n" << "Incorrect argument count, expects " << f.prototype().count() << " but " << d->arguments.size() << " were provided";
    else if(status == CouldNotConvertArgument)
      diag << "\n" << "Could not convert argument " << convs.size();
    diag << "\n";
  }
  return diag;
}

std::string OverloadResolution::dtype(const Type & t) const
{
  std::string ret = d->engine->typeName(t);
  if (t.isConst())
    ret = std::string{ "const " } + ret;
  if (t.isReference())
    ret = ret + "&";
  else if (t.isRefRef())
    ret = ret + "&&";
  return ret;
}

void OverloadResolution::write_prototype(diagnostic::MessageBuilder & diag, const Function & f) const
{
  if (f.isConstructor())
    diag << f.memberOf().name();
  else if (f.isDestructor())
    diag << "~" << f.memberOf().name();
  else if (f.isCast())
  {
    diag << "operator " << dtype(f.returnType());
  }
  else if (f.isOperator())
  {
    diag << dtype(f.returnType());
    diag << " operator" << Operator::getSymbol(f.toOperator().operatorId());
  }
  else 
  {
    diag << dtype(f.returnType()) << " " << f.name();
  }

  diag << "(";
  for (int i(0); i < f.prototype().count(); ++i)
  {
    diag << dtype(f.parameter(i));
    if (i < f.prototype().count() - 1)
      diag << ", ";
  }
  diag << ")";
}

bool OverloadResolution::process(const std::vector<Function> & candidates, const Arguments & arguments)
{
  d->candidates = &candidates;
  d->arguments = arguments;
  d->implicit_object = nullptr;

  const int argc = arguments.size();

  std::vector<ConversionSequence> conversions;

  for (const auto & func : candidates)
  {
    conversions.clear();
    if (!func.accepts(argc))
      continue;

    bool ok = true;
    for (int i(0); i < argc; ++i)
    {
      ConversionSequence conv = arguments.conversion(i, func.parameter(i), d->engine);
      conversions.push_back(conv);
      if (conv == ConversionSequence::NotConvertible())
      {
        ok = false;
        break;
      }
    }

    if (!ok)
      continue;

    processCandidate(func, conversions);
  }

  if (d->ambiguous.isNull() && !d->selected.isNull())
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

  d->candidates = &candidates;
  d->arguments = Arguments(&arguments);
  d->implicit_object = object;

  const int argc = arguments.size();

  std::vector<ConversionSequence> conversions;

  for (const auto & func : candidates)
  {
    conversions.clear();
    const int actual_argc = func.hasImplicitObject() ? argc + 1 : argc;

    if (!func.accepts(actual_argc))
      continue;

    if (func.hasImplicitObject())
    {
      ConversionSequence conv = ConversionSequence::compute(object->type(), func.parameter(0), d->engine);
      conversions.push_back(conv);
      if (conv == ConversionSequence::NotConvertible() || conv.conv1.isCopyInitialization())
        continue;
    }

    const int parameter_offset = func.hasImplicitObject() ? 1 : 0;

    bool ok = true;
    for (int i(0); i < argc; ++i)
    {
      ConversionSequence conv = ConversionSequence::compute(arguments.at(i), func.parameter(i + parameter_offset), d->engine);
      conversions.push_back(conv);
      if (conv == ConversionSequence::NotConvertible())
      {
        ok = false;
        break;
      }
    }

    if (!ok)
      continue;

    processCandidate(func, conversions);
  }

  if (d->ambiguous.isNull() && !d->selected.isNull())
    return true;
  return false;
}

void OverloadResolution::processCandidate(const Function & f, std::vector<ConversionSequence> & conversions)
{
  if (f == d->selected || f == d->ambiguous)
    return;

  OverloadComparison comp = compare(f, conversions, d->selected, d->selected_conversions);
  if (comp == Indistinguishable || comp == NotComparable)
  {
    d->ambiguous = f;
    std::swap(d->alternative_conversions, conversions);
  }
  else if (comp == FirstIsBetter)
  {
    d->selected = f;
    std::swap(d->selected_conversions, conversions);

    if (!d->ambiguous.isNull())
    {
      comp = compare(d->selected, d->selected_conversions, d->ambiguous, d->alternative_conversions);
      if (comp == FirstIsBetter)
        d->ambiguous = Function{};
    }
  }
  else if (comp == SecondIsBetter)
  {
    if (!d->ambiguous.isNull())
    {
      comp = compare(f, conversions, d->ambiguous, d->alternative_conversions);
      if (comp == FirstIsBetter)
      {
        d->ambiguous = f;
        std::swap(d->alternative_conversions, conversions);
      }
    }
  }
}

OverloadResolution::OverloadComparison OverloadResolution::compare(const Function & a, const std::vector<ConversionSequence> & conv_a, const Function & b, const std::vector<ConversionSequence> & conv_b)
{
  if (b.isNull() && !a.isNull())
    return FirstIsBetter;
  else if (a.isNull() && !b.isNull())
    return SecondIsBetter;

  if (a.isNull() && b.isNull())
    return Indistinguishable;

  assert(!a.isNull() && !b.isNull());

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

  for (int i(first_diff_index + 1); i < nb_conversions; ++i)
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

Operator OverloadResolution::select(const std::vector<Function> & candidates, const Type & lhs, const Type & rhs)
{
  if (candidates.empty())
    return Operator{};

  OverloadResolution resol = OverloadResolution::New(candidates.front().engine(), NoOptions);
  if (resol.process(candidates, std::vector<Type>{lhs, rhs}))
    return resol.selectedOverload().toOperator();
  return Operator{};
}

} // namespace script