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
class DiagnosticMessage;
class MessageBuilder;
} // namespace diagnostic

class Initialization;

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
  const std::vector<Initialization> & initializations() const;

  Function ambiguousOverload() const;

  const std::vector<Function> & candidates() const;

  enum ViabilityStatus {
    Viable,
    IncorrectParameterCount,
    CouldNotConvertArgument,
  };

  ViabilityStatus getViabilityStatus(const Function & f, std::vector<Initialization> *conversions = nullptr) const;
  ViabilityStatus getViabilityStatus(int candidate_index, std::vector<Initialization> *conversions = nullptr) const;

  enum InputKind {
    NullInputs = 0,
    TypeInputs,
    ValueInputs,
    ExpressionInputs,
  };

  InputKind inputKind() const;
  const std::vector<Type> & typeInputs() const;
  const std::vector<Value> & valueInputs() const;
  const std::vector<std::shared_ptr<program::Expression>> & expressionInputs() const;

  const std::shared_ptr<program::Expression> & implicit_object() const;

  diagnostic::DiagnosticMessage emitDiagnostic() const;

  bool process(const std::vector<Function> & candidates, const std::vector<Type> & types);
  bool process(const std::vector<Function> & candidates, const std::vector<Value> & values);
  bool process(const std::vector<Function> & candidates, const std::vector<std::shared_ptr<program::Expression>> & arguments);
  bool process(const std::vector<Function> & candidates, const std::vector<std::shared_ptr<program::Expression>> & arguments, const std::shared_ptr<program::Expression> & object);

  bool processConstructors(const std::vector<Function> & candidates, const std::vector<Value> & values);

  enum OverloadComparison {
    FirstIsBetter = 1,
    SecondIsBetter = 2,
    Indistinguishable = 3,
    NotComparable = 4,
  };

  /// TODO: is passing an Engine* here absolutely necessary ?
  static OverloadResolution New(Engine *engine, int options = 0);

  static Function select(const std::vector<Function> & candidates, const std::vector<Type> & types);
  static Function select(const std::vector<Function> & candidates, const std::vector<Value> & args);
  static Function selectConstructor(const std::vector<Function> & candidates, const std::vector<Value> & args);

  OverloadResolution & operator=(const OverloadResolution & other) = default;

protected:
  /// TODO: move elsewhere
  std::string dtype(const Type & t) const;
  void write_prototype(diagnostic::MessageBuilder & diag, const Function & f) const;

private:
  std::shared_ptr<OverloadResolutionImpl> d;
};

} // namespace script

#endif // LIBSCRIPT_OVERLOAD_RESOLUTION_H
