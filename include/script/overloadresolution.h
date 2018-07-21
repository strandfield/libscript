// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_OVERLOAD_RESOLUTION_H
#define LIBSCRIPT_OVERLOAD_RESOLUTION_H

#include "script/engine.h" /// TODO: forward declare
#include "script/function.h"

namespace script
{

namespace diagnostic
{
class Message;
class MessageBuilder;
} // namespace diagnostic

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

  // no options for now
  enum Option {
    NoOptions = 0,
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

  const std::vector<Function> & candidates() const;

  enum ViabilityStatus {
    Viable,
    IncorrectParameterCount,
    CouldNotConvertArgument,
  };

  ViabilityStatus getViabilityStatus(const Function & f, std::vector<ConversionSequence> *conversions = nullptr) const;
  ViabilityStatus getViabilityStatus(int candidate_index, std::vector<ConversionSequence> *conversions = nullptr) const;

  class Arguments
  {
  public:
    Arguments();
    Arguments(const std::vector<Type> *types);
    Arguments(const std::vector<Value> *values);
    Arguments(const std::vector<std::shared_ptr<program::Expression>> *exprs);
    Arguments(const Arguments &) = default;
    ~Arguments() = default;

    inline bool is_null() const { return mTypes == nullptr && mValues == nullptr && mExpressions == nullptr; }

    enum Kind {
      Null = 0,
      TypeArguments,
      ValueArguments,
      ExpressionArguments,
    };

    Kind kind() const;

    const std::vector<Type> & types() const;
    const std::vector<Value> & values() const;
    const std::vector<std::shared_ptr<program::Expression>> & expressions() const;
    
    int size() const;
    ConversionSequence conversion(int argIndex, const Type & dest, Engine *engine) const;

    Arguments & operator=(const Arguments & other);

  private:
    const std::vector<Type> *mTypes;
    const std::vector<Value> *mValues;
    const std::vector<std::shared_ptr<program::Expression>> *mExpressions;
  };

  const Arguments & arguments() const;
  const std::shared_ptr<program::Expression> & implicit_object() const;

  diagnostic::Message emitDiagnostic() const;

  bool process(const std::vector<Function> & candidates, const Arguments & types);
  bool process(const std::vector<Function> & candidates, const std::vector<Type> & types);
  bool process(const std::vector<Function> & candidates, const std::vector<std::shared_ptr<program::Expression>> & arguments);
  bool process(const std::vector<Function> & candidates, const std::vector<std::shared_ptr<program::Expression>> & arguments, const std::shared_ptr<program::Expression> & object);


  enum OverloadComparison {
    FirstIsBetter = 1,
    SecondIsBetter = 2,
    Indistinguishable = 3,
    NotComparable = 4,
  };

  static OverloadComparison compare(const Function & a, const std::vector<ConversionSequence> & conv_a, const Function & b, const std::vector<ConversionSequence> & conv_b);

  /// TODO: is passing an Engine* here absolutely necessary ?
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
  static Operator select(const std::vector<Function> & candidates, const Type & lhs, const Type & rhs);


  OverloadResolution & operator=(const OverloadResolution & other) = default;

protected:
  void processCandidate(const Function & f, std::vector<ConversionSequence> & conversions);

  std::string dtype(const Type & t) const;
  void write_prototype(diagnostic::MessageBuilder & diag, const Function & f) const;

private:
  std::shared_ptr<OverloadResolutionImpl> d;
};

} // namespace script

#endif // LIBSCRIPT_OVERLOAD_RESOLUTION_H
