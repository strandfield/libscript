// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_OVERLOAD_RESOLUTION_H
#define LIBSCRIPT_OVERLOAD_RESOLUTION_H

#include "script/engine.h"
#include "script/function.h"

namespace script
{

struct OverloadResolutionImpl;

// see http://en.cppreference.com/w/cpp/language/overload_resolution
// http://en.cppreference.com/w/cpp/language/implicit_conversion
// http://en.cppreference.com/w/cpp/language/cast_operator

class LIBSCRIPT_API OverloadResolution
{
public:
  OverloadResolution();
  OverloadResolution(const OverloadResolution &) = default;
  ~OverloadResolution() = default;

  enum Option {
    NoOptions = 0,
    StoreCandidates = 1,
  };

  inline bool isNull() const { return d == nullptr; }
  inline bool isValid() const { return !isNull(); }

  void setOption(Option opt, bool on = true);
  bool testOption(Option opt) const;
  int options() const;
  
  bool success() const;
  inline bool failure() const { return !success(); }

  Function selectedOverload() const;
  const std::vector<ConversionSequence> & conversionSequence() const;

  Function ambiguousOverload() const;

  enum ViabilityStatus {
    Viable,
    IncorrectParameterCount,
    CouldNotConvertArgument,
  };

  struct Candidate
  {
    Candidate();
    Candidate(const Candidate & ) = default;

    Candidate(const Function & f, ViabilityStatus status = Viable);

    bool isViable() const;
    ViabilityStatus viabilityStatus() const;
    int argument() const;
    Function function;
    int vastatus;
    int mArgument;
  };

  const std::vector<Candidate> & candidates() const;

  class Arguments
  {
  public:
    Arguments(const std::vector<Type> *types);
    Arguments(const std::vector<Value> *values);
    Arguments(const std::vector<std::shared_ptr<program::Expression>> *exprs);
    Arguments(const Arguments &) = delete;
    ~Arguments() = default;

    int size() const;
    ConversionSequence conversion(int argIndex, const Type & dest, Engine *engine) const;

  private:
    const std::vector<Type> *mTypes;
    const std::vector<Value> *mValues;
    const std::vector<std::shared_ptr<program::Expression>> *mExpressions;
  };

  bool process(const std::vector<Function> & candidates, const Arguments & types);
  bool process(const std::vector<Function> & candidates, const std::vector<Type> & types);
  bool process(const std::vector<Function> & candidates, const std::vector<std::shared_ptr<program::Expression>> & arguments);
  bool process(const std::vector<Function> & candidates, const std::vector<std::shared_ptr<program::Expression>> & arguments, const std::shared_ptr<program::Expression> & object);
  bool process(const std::vector<Operator> & candidates, const Type & lhs, const Type & rhs);
  bool process(const std::vector<Operator> & candidates, const Type & arg);


  enum OverloadComparison {
    FirstIsBetter = 1,
    SecondIsBetter = 2,
    Indistinguishable = 3,
    NotComparable = 4,
  };
  static OverloadComparison compare(const Candidate & a, const std::vector<ConversionSequence> & conv_a, const Candidate & b, const std::vector<ConversionSequence> & conv_b);

  static OverloadResolution New(Engine *engine, int options = 0);

  static Function select(const std::vector<Function> & candidates, const std::vector<Type> & types)
  {
    return select(candidates, Arguments(&types));
  }

  inline static Function select(const std::vector<Function> & candidates, const std::vector<Value> & args)
  {
    return select(candidates, Arguments(&args));
  }

  static Function select(const std::vector<Function> & candidates, const Arguments & types);
  static Operator select(const std::vector<Operator> & candidates, const Type & lhs, const Type & rhs);


  OverloadResolution & operator=(const OverloadResolution & other) = default;

protected:
  void processCandidate(const Candidate & c, std::vector<ConversionSequence> & conversions);

private:
  std::shared_ptr<OverloadResolutionImpl> d;
};

} // namespace script

#endif // LIBSCRIPT_OVERLOAD_RESOLUTION_H
