// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/overloadresolution.h"

#include "script/diagnosticmessage.h"
#include "script/class.h"
#include "script/conversions.h"
#include "script/engine.h"
#include "script/initialization.h"
#include "script/operator.h"

#include "script/program/expression.h"

namespace script
{

struct ORCandidate
{
  Function function;
  std::vector<Initialization> initializations;

public:
  ORCandidate() = default;
  ORCandidate(const ORCandidate &) = delete;
  ORCandidate(ORCandidate &&) = default;
  ~ORCandidate() = default;

  ORCandidate & operator=(ORCandidate &) = delete;
  ORCandidate & operator=(ORCandidate &&) = default;

  inline void set(const Function & f)
  {
    function = f;
    initializations.clear();
  }

  inline void reset()
  {
    function = Function{};
    initializations.clear();
  }
};

struct OverloadResolutionImpl
{
  Engine *engine;
  int options;
  std::vector<Function> const *candidates;
  OverloadResolution::Arguments arguments;
  std::shared_ptr<program::Expression> implicit_object;

  ORCandidate selected_candidate;
  ORCandidate ambiguous_candidate;
  ORCandidate current_candidate;

  OverloadResolutionImpl(Engine *e, int opts)
    : engine(e)
    , options(opts)
    , candidates(nullptr)
  {

  }

  static OverloadResolution::OverloadComparison compare(const ORCandidate & a, const ORCandidate & b);
  void processCurrentCandidate();
};



OverloadResolution::OverloadComparison OverloadResolutionImpl::compare(const ORCandidate & a, const ORCandidate & b)
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

  const int nb_conversions = a.initializations.size();
  int first_diff = 0;
  int first_diff_index = -1;
  for (int i(0); i < nb_conversions; ++i)
  {
    first_diff = Initialization::comp(a.initializations.at(i), b.initializations.at(i));
    if (first_diff)
    {
      first_diff_index = i;
      break;
    }
  }

  if (first_diff_index == -1)
    return OverloadResolution::Indistinguishable;

  for (int i(first_diff_index + 1); i < nb_conversions; ++i)
  {
    const int diff = Initialization::comp(a.initializations.at(i), b.initializations.at(i));
    if (diff == -first_diff)
      return OverloadResolution::NotComparable;
  }

  return first_diff == -1 ? OverloadResolution::FirstIsBetter : OverloadResolution::SecondIsBetter;
}

void OverloadResolutionImpl::processCurrentCandidate()
{
  if (current_candidate.function == selected_candidate.function || current_candidate.function == ambiguous_candidate.function)
    return;

  OverloadResolution::OverloadComparison comp = compare(current_candidate, selected_candidate);
  if (comp == OverloadResolution::Indistinguishable || comp == OverloadResolution::NotComparable)
  {
    std::swap(ambiguous_candidate, current_candidate);
  }
  else if (comp == OverloadResolution::FirstIsBetter)
  {
    std::swap(selected_candidate, current_candidate);

    if (!ambiguous_candidate.function.isNull())
    {
      comp = compare(selected_candidate, ambiguous_candidate);
      if (comp == OverloadResolution::FirstIsBetter)
        ambiguous_candidate.reset();
    }
  }
  else if (comp == OverloadResolution::SecondIsBetter)
  {
    if (!ambiguous_candidate.function.isNull())
    {
      comp = compare(current_candidate, ambiguous_candidate);
      if (comp == OverloadResolution::FirstIsBetter)
      {
        std::swap(ambiguous_candidate, current_candidate);
      }
    }
  }
}


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

Initialization OverloadResolution::Arguments::initialization(int argIndex, const Type & parametertype, Engine *engine) const
{
  if (mTypes != nullptr)
    return Initialization::compute(parametertype, mTypes->at(argIndex), engine, Initialization::CopyInitialization);
  else if (mValues != nullptr)
    return Initialization::compute(parametertype, mValues->at(argIndex).type(), engine, Initialization::CopyInitialization);
  return Initialization::compute(parametertype, mExpressions->at(argIndex), engine);
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
  return !d->selected_candidate.function.isNull() && d->ambiguous_candidate.function.isNull();
}

Function OverloadResolution::selectedOverload() const
{
  return d->selected_candidate.function;
}

const std::vector<Initialization> & OverloadResolution::initializations() const
{
  return d->selected_candidate.initializations;
}

Function OverloadResolution::ambiguousOverload() const
{
  return d->ambiguous_candidate.function;
}

const std::vector<Function> & OverloadResolution::candidates() const
{
  return *(d->candidates);
}

OverloadResolution::ViabilityStatus OverloadResolution::getViabilityStatus(const Function & f, std::vector<Initialization> *conversions) const
{
  std::vector<Initialization> convs;
  if (conversions == nullptr)
    conversions = &convs;

  conversions->clear();

  const int implicit_object_offset = (f.hasImplicitObject() && d->implicit_object != nullptr) ? 1 : 0;
  const int argc = d->arguments.size() + implicit_object_offset;

  if (!f.accepts(argc))
    return IncorrectParameterCount;

  if (implicit_object_offset)
  {
    Initialization ini = Initialization::compute(f.parameter(0), d->implicit_object->type(), d->engine);
    conversions->push_back(ini);
    if (!ini.isValid() || ini.conversion().firstStandardConversion().isCopy())
      return CouldNotConvertArgument;
  }

  for (int i(0); i < argc; ++i)
  {
    Initialization ini = d->arguments.initialization(i, f.parameter(i + implicit_object_offset), d->engine);
    conversions->push_back(ini);
    if (!ini.isValid())
      return CouldNotConvertArgument;
  }

  return Viable;
}

OverloadResolution::ViabilityStatus OverloadResolution::getViabilityStatus(int candidate_index, std::vector<Initialization> *conversions) const
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

  if (!d->ambiguous_candidate.function.isNull())
  {
    auto diag = diagnostic::info() << "Overload resolution failed because at least two candidates are not comparable or indistinguishable \n";
    write_prototype(diag, d->selected_candidate.function);
    diag << "\n";
    write_prototype(diag, d->ambiguous_candidate.function);
    return diag;
  }
  
  auto diag = diagnostic::info() << "Overload resolution failed because no viable overload could be found \n";
  std::vector<Initialization> paraminits;
  for (const auto & f : *(d->candidates))
  {
    auto status = getViabilityStatus(f, &paraminits);
    write_prototype(diag, f);
    if (status == IncorrectParameterCount)
      diag << "\n" << "Incorrect argument count, expects " << f.prototype().count() << " but " << d->arguments.size() << " were provided";
    else if(status == CouldNotConvertArgument)
      diag << "\n" << "Could not convert argument " << paraminits.size();
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

bool OverloadResolution::process(const std::vector<Function> & candidates, const std::vector<Type> & types)
{
  return process(candidates, Arguments{ &types });
}

bool OverloadResolution::process(const std::vector<Function> & candidates, const Arguments & arguments)
{
  d->candidates = &candidates;
  d->arguments = arguments;
  d->implicit_object = nullptr;

  const int argc = arguments.size();

  for (const auto & func : candidates)
  {
    d->current_candidate.set(func);
    if (!func.accepts(argc))
      continue;

    bool ok = true;
    for (int i(0); i < argc; ++i)
    {
      Initialization init = arguments.initialization(i, func.parameter(i), d->engine);
      d->current_candidate.initializations.push_back(init);
      if (init.kind() == Initialization::InvalidInitialization)
      {
        ok = false;
        break;
      }
    }

    if (!ok)
      continue;

    d->processCurrentCandidate();
  }

  if (d->ambiguous_candidate.function.isNull() && !d->selected_candidate.function.isNull())
    return true;
  return false;
}

bool OverloadResolution::process(const std::vector<Function> & candidates, const Arguments & arguments, const Type & obj)
{
  d->candidates = &candidates;
  d->arguments = arguments;

  const int argc = arguments.size();

  for (const auto & func : candidates)
  {
    d->current_candidate.set(func);
    const int actual_argc = func.hasImplicitObject() ? argc + 1 : argc;

    if (!func.accepts(actual_argc))
      continue;

    if (func.hasImplicitObject())
    {
      Conversion conv = Conversion::compute(obj, func.parameter(0), d->engine);
      if (conv == Conversion::NotConvertible() || conv.firstStandardConversion().isCopy())
        continue;
      d->current_candidate.initializations.push_back(Initialization{ Initialization::DirectInitialization, conv });
    }

    const int parameter_offset = func.hasImplicitObject() ? 1 : 0;

    bool ok = true;
    for (int i(0); i < argc; ++i)
    {
      Initialization init = arguments.initialization(i, func.parameter(i + parameter_offset), d->engine);
      if (init.kind() == Initialization::InvalidInitialization)
      {
        ok = false;
        break;
      }
      d->current_candidate.initializations.push_back(init);
    }

    if (!ok)
      continue;

    d->processCurrentCandidate();
  }

  if (d->ambiguous_candidate.function.isNull() && !d->selected_candidate.function.isNull())
    return true;
  return false;
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

  for (const auto & func : candidates)
  {
    d->current_candidate.set(func);
    const int actual_argc = func.hasImplicitObject() ? argc + 1 : argc;

    if (!func.accepts(actual_argc))
      continue;

    if (func.hasImplicitObject())
    {
      Conversion conv = Conversion::compute(object->type(), func.parameter(0), d->engine);
      if (conv == Conversion::NotConvertible() || conv.firstStandardConversion().isCopy())
        continue;
      d->current_candidate.initializations.push_back(Initialization{ Initialization::DirectInitialization, conv });
    }

    const int parameter_offset = func.hasImplicitObject() ? 1 : 0;

    bool ok = true;
    for (int i(0); i < argc; ++i)
    {
      Initialization init = Initialization::compute(func.parameter(i + parameter_offset), arguments.at(i), d->engine);
      if (init.kind() == Initialization::InvalidInitialization)
      {
        ok = false;
        break;
      }
      d->current_candidate.initializations.push_back(init);
    }

    if (!ok)
      continue;

    d->processCurrentCandidate();
  }

  if (d->ambiguous_candidate.function.isNull() && !d->selected_candidate.function.isNull())
    return true;
  return false;
}

OverloadResolution OverloadResolution::New(Engine *engine, int options)
{
  OverloadResolution ret;
  ret.d = std::make_shared<OverloadResolutionImpl>(engine, options);
  return ret;
}

Function OverloadResolution::select(const std::vector<Function> & candidates, const std::vector<Value> & args, const Value & obj)
{
  return select(candidates, Arguments(&args), obj.type());
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

Function OverloadResolution::select(const std::vector<Function> & candidates, const Arguments & types, const Type & obj)
{
  if (candidates.empty())
    return Function{};

  OverloadResolution resol = OverloadResolution::New(candidates.front().engine(), NoOptions);
  if (resol.process(candidates, types, obj))
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