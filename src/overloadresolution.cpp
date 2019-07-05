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
#include "script/typesystem.h"

#include "script/program/expression.h"

namespace script
{

struct ORInputs
{
public:
  ORInputs();
  ORInputs(const ORInputs &) = delete;
  ~ORInputs() = default;

  inline bool is_null() const { return this->kind == OverloadResolution::NullInputs; }

  void set(const std::vector<Type> & types_)
  {
    kind = OverloadResolution::TypeInputs;
    types = &types_;
  }

  void set(const std::vector<Value> & vals)
  {
    kind = OverloadResolution::ValueInputs;
    values = &vals;
  }

  void set(const std::vector<std::shared_ptr<program::Expression>> & expr)
  {
    kind = OverloadResolution::ExpressionInputs;
    expressions = &expr;
  }

  int size() const
  {
    switch (kind)
    {
    case OverloadResolution::TypeInputs:
      return types->size();
    case OverloadResolution::ValueInputs:
      return values->size();
    case OverloadResolution::ExpressionInputs:
      return expressions->size();
    }

    return 0;
  }

  Initialization initialization(int argIndex, const Type & parametertype, Engine *e) const
  {
    switch (kind)
    {
    case OverloadResolution::TypeInputs:
      return Initialization::compute(parametertype, types->at(argIndex), e, Initialization::CopyInitialization);
    case OverloadResolution::ValueInputs:
      return Initialization::compute(parametertype, values->at(argIndex).type(), e, Initialization::CopyInitialization);
    case OverloadResolution::ExpressionInputs:
      return Initialization::compute(parametertype, expressions->at(argIndex), e);
    }

    return {};
  }

  int kind;
  const std::vector<Type> *types;
  const std::vector<Value> *values;
  const std::vector<std::shared_ptr<program::Expression>> *expressions;
};

ORInputs::ORInputs()
  : kind(OverloadResolution::NullInputs)
  , types(nullptr)
  , values(nullptr)
  , expressions(nullptr)
{

}


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
  std::shared_ptr<program::Expression> implicit_object;

  ORInputs inputs;

  ORCandidate selected_candidate;
  ORCandidate ambiguous_candidate;
  ORCandidate current_candidate;

  OverloadResolutionImpl(Engine *e, int opts)
    : engine(e)
    , options(opts)
    , candidates(nullptr)
  {

  }

  bool processOverloadResolution();
  bool processORWithoutObject(int nbIgnored);

  static OverloadResolution::OverloadComparison compare(const ORCandidate & a, const ORCandidate & b);
  void processCurrentCandidate();
};

bool OverloadResolutionImpl::processOverloadResolution()
{
  if (implicit_object == nullptr)
    return processORWithoutObject(0);

  const int argc = inputs.size();

  for (const auto & func : *candidates)
  {
    current_candidate.set(func);
    const int actual_argc = func.hasImplicitObject() ? argc + 1 : argc;

    if (actual_argc > func.prototype().count() || actual_argc + int(func.defaultArguments().size()) < func.prototype().count())
      continue;

    if (func.hasImplicitObject())
    {
      Conversion conv = Conversion::compute(implicit_object->type(), func.parameter(0), engine);
      if (conv == Conversion::NotConvertible() || conv.firstStandardConversion().isCopy())
        continue;
      current_candidate.initializations.push_back(Initialization{ Initialization::DirectInitialization, conv });
    }

    const int parameter_offset = func.hasImplicitObject() ? 1 : 0;

    bool ok = true;
    for (int i(0); i < argc; ++i)
    {
      Initialization init = inputs.initialization(i, func.parameter(i + parameter_offset), engine);
      if (init.kind() == Initialization::InvalidInitialization)
      {
        ok = false;
        break;
      }
      current_candidate.initializations.push_back(init);
    }

    if (!ok)
      continue;

    processCurrentCandidate();
  }

  if (ambiguous_candidate.function.isNull() && !selected_candidate.function.isNull())
    return true;
  return false;
}

bool OverloadResolutionImpl::processORWithoutObject(int nbIgnored)
{
  const int argc = inputs.size() + nbIgnored;

  for (const auto & func : *candidates)
  {
    current_candidate.set(func);

    if (argc > func.prototype().count() || argc + int(func.defaultArguments().size()) < func.prototype().count())
      continue;

    bool ok = true;
    for (int i(nbIgnored); i < argc; ++i)
    {
      Initialization init = inputs.initialization(i - nbIgnored, func.parameter(i), engine);
      if (init.kind() == Initialization::InvalidInitialization)
      {
        ok = false;
        break;
      }
      current_candidate.initializations.push_back(init);
    }

    if (!ok)
      continue;

    processCurrentCandidate();
  }

  if (ambiguous_candidate.function.isNull() && !selected_candidate.function.isNull())
    return true;
  return false;
}

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
  const int argc = d->inputs.size() + implicit_object_offset;

  if (argc > f.prototype().count() || argc + int(f.defaultArguments().size()) < f.prototype().count())
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
    Initialization ini = d->inputs.initialization(i, f.parameter(i + implicit_object_offset), d->engine);
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

OverloadResolution::InputKind OverloadResolution::inputKind() const
{
  return static_cast<InputKind>(d->inputs.kind);
}

const std::vector<Type> & OverloadResolution::typeInputs() const
{
  return *(d->inputs.types);
}

const std::vector<Value> & OverloadResolution::valueInputs() const
{
  return *(d->inputs.values);
}

const std::vector<std::shared_ptr<program::Expression>> & OverloadResolution::expressionInputs() const
{
  return *(d->inputs.expressions);
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
      diag << "\n" << "Incorrect argument count, expects " << f.prototype().count() << " but " << d->inputs.size() << " were provided";
    else if(status == CouldNotConvertArgument)
      diag << "\n" << "Could not convert argument " << paraminits.size();
    diag << "\n";
  }
  return diag;
}

std::string OverloadResolution::dtype(const Type & t) const
{
  std::string ret = d->engine->typeSystem()->typeName(t);
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
  d->candidates = &candidates;
  d->implicit_object = nullptr;
  d->inputs.set(types);
  return d->processOverloadResolution();
}

bool OverloadResolution::process(const std::vector<Function> & candidates, const std::vector<Value> & values)
{
  d->candidates = &candidates;
  d->implicit_object = nullptr;
  d->inputs.set(values);
  return d->processOverloadResolution();
}

bool OverloadResolution::process(const std::vector<Function> & candidates, const std::vector<std::shared_ptr<program::Expression>> & arguments)
{
  d->candidates = &candidates;
  d->implicit_object = nullptr;
  d->inputs.set(arguments);
  return d->processOverloadResolution();
}

bool OverloadResolution::process(const std::vector<Function> & candidates, const std::vector<std::shared_ptr<program::Expression>> & arguments, const std::shared_ptr<program::Expression> & object)
{
  if (object == nullptr)
    return process(candidates, arguments);

  d->candidates = &candidates;
  d->implicit_object = object;
  d->inputs.set(arguments);
  
  return d->processOverloadResolution();
}

bool OverloadResolution::processConstructors(const std::vector<Function> & candidates, const std::vector<Value> & values)
{
  d->candidates = &candidates;
  d->implicit_object = nullptr;
  d->inputs.set(values);
  const int nbIgnored = 1;
  return d->processORWithoutObject(nbIgnored);
}

OverloadResolution OverloadResolution::New(Engine *engine, int options)
{
  OverloadResolution ret;
  ret.d = std::make_shared<OverloadResolutionImpl>(engine, options);
  return ret;
}

Function OverloadResolution::select(const std::vector<Function> & candidates, const std::vector<Type> & types)
{
  if (candidates.empty())
    return Function{};

  OverloadResolution resol = OverloadResolution::New(candidates.front().engine(), NoOptions);
  if (resol.process(candidates, types))
    return resol.selectedOverload();
  return Function{};
}

Function OverloadResolution::select(const std::vector<Function> & candidates, const std::vector<Value> & args)
{
  if (candidates.empty())
    return Function{};

  OverloadResolution resol = OverloadResolution::New(candidates.front().engine(), NoOptions);
  if (resol.process(candidates, args))
    return resol.selectedOverload();
  return Function{};
}

Function OverloadResolution::selectConstructor(const std::vector<Function> & candidates, const std::vector<Value> & args)
{
  if (candidates.empty())
    return Function{};

  OverloadResolution resol = OverloadResolution::New(candidates.front().engine(), NoOptions);
  if (resol.processConstructors(candidates, args))
    return resol.selectedOverload();
  return Function{};
}

} // namespace script