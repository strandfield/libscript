// Copyright (C) 2018-2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_OVERLOAD_RESOLUTION_H
#define LIBSCRIPT_OVERLOAD_RESOLUTION_H

#include "script/function.h"
#include "script/initialization.h"
#include "script/value.h"
#include "script/overload-resolution-helper.h"

namespace script
{

// see http://en.cppreference.com/w/cpp/language/overload_resolution
// http://en.cppreference.com/w/cpp/language/implicit_conversion
// http://en.cppreference.com/w/cpp/language/cast_operator

class LIBSCRIPT_API OverloadResolution
{
public:
  OverloadResolution();
  OverloadResolution(const OverloadResolution &) = default;
  ~OverloadResolution() = default;

  struct Candidate
  {
    Function function;
    std::vector<Initialization> initializations;

  public:
    Candidate() = default;
    Candidate(const Candidate&) = delete;
    Candidate(Candidate&&) noexcept = default;
    ~Candidate() = default;

    Candidate& operator=(Candidate&) = delete;
    Candidate& operator=(Candidate&&) noexcept = default;

    inline void set(const Function& f)
    {
      function = f;
      initializations.clear();
    }

    inline void reset()
    {
      function = Function{};
      initializations.clear();
    }

    operator bool() const { return !function.isNull(); }
  };

  enum OverloadComparison {
    FirstIsBetter = 1,
    SecondIsBetter = 2,
    Indistinguishable = 3,
    NotComparable = 4,
  };

  static OverloadComparison compare(const Candidate& a, const Candidate& b);
};


template<>
struct overload_resolution_helper<Type>
{
  static bool is_null(const Type& type) { return type.isNull(); }
  static Type get_type(const Type& type) { return type; }
  static Initialization init(const Type& parameter_type, const Type& argtype, Engine* e) { return Initialization::compute(parameter_type, argtype, e, Initialization::CopyInitialization); }
};

template<>
struct overload_resolution_helper<Value>
{
  static bool is_null(const Value& v) { return v.isNull(); }
  static Type get_type(const Value& v) { return v.type(); }
  static Initialization init(const Type& parameter_type, const Value& val, Engine* e) { return Initialization::compute(parameter_type, val.type(), e, Initialization::CopyInitialization); }
};

namespace details
{

inline void overloadresolution_process_candidate(OverloadResolution::Candidate& current, OverloadResolution::Candidate& selected, OverloadResolution::Candidate& ambiguous)
{
  if (current.function == selected.function || current.function == ambiguous.function)
    return;

  OverloadResolution::OverloadComparison comp = OverloadResolution::compare(current, selected);
  if (comp == OverloadResolution::Indistinguishable || comp == OverloadResolution::NotComparable)
  {
    std::swap(ambiguous, current);
  }
  else if (comp == OverloadResolution::FirstIsBetter)
  {
    std::swap(selected, current);

    if (!ambiguous.function.isNull())
    {
      comp = OverloadResolution::compare(selected, ambiguous);
      if (comp == OverloadResolution::FirstIsBetter)
        ambiguous.reset();
    }
  }
  else if (comp == OverloadResolution::SecondIsBetter)
  {
    if (!ambiguous.function.isNull())
    {
      comp = OverloadResolution::compare(current, ambiguous);
      if (comp == OverloadResolution::FirstIsBetter)
      {
        std::swap(ambiguous, current);
      }
    }
  }
}

} // namespace details

// @TODO: add an extra template parameter for diagnostics

template<typename T>
OverloadResolution::Candidate resolve_overloads(const std::vector<Function>& candidates, const std::vector<T>& args)
{
  OverloadResolution::Candidate current;
  OverloadResolution::Candidate selected;
  OverloadResolution::Candidate ambiguous;

  const size_t argc = args.size();

  for (const auto& func : candidates)
  {
    Engine* engine = func.engine();

    current.set(func);

    if (argc > func.prototype().count() || argc + int(func.defaultArguments().size()) < func.prototype().count())
      continue;

    bool ok = true;
    for (size_t i(0); i < argc; ++i)
    {
      Initialization init = overload_resolution_helper<T>::init(func.parameter(static_cast<int>(i)), args.at(i), engine);
      if (init.kind() == Initialization::InvalidInitialization)
      {
        ok = false;
        break;
      }
      current.initializations.push_back(init);
    }

    if (!ok)
      continue;

    details::overloadresolution_process_candidate(current, selected, ambiguous);
  }

  if (ambiguous.function.isNull() && !selected.function.isNull())
    return selected;
  return {};
}

template<typename T, typename U>
OverloadResolution::Candidate resolve_overloads(const std::vector<Function>& candidates, const T& implicit_object, const std::vector<U>& args)
{
  if (overload_resolution_helper<T>::is_null(implicit_object))
    return resolve_overloads(candidates, args);

  OverloadResolution::Candidate current;
  OverloadResolution::Candidate selected;
  OverloadResolution::Candidate ambiguous;

  const int argc = static_cast<int>(args.size());

  for (const auto& func : candidates)
  {
    Engine* engine = func.engine();

    current.set(func);
    const int actual_argc = func.hasImplicitObject() ? argc + 1 : argc;

    if (actual_argc > func.prototype().count() || actual_argc + int(func.defaultArguments().size()) < func.prototype().count())
      continue;

    if (func.hasImplicitObject())
    {
      Conversion conv = Conversion::compute(overload_resolution_helper<T>::get_type(implicit_object), func.parameter(0), engine);
      if (conv == Conversion::NotConvertible() || conv.firstStandardConversion().isCopy())
        continue;
      current.initializations.push_back(Initialization{ Initialization::DirectInitialization, conv });
    }

    const int parameter_offset = func.hasImplicitObject() ? 1 : 0;

    bool ok = true;
    for (int i(0); i < argc; ++i)
    {
      Initialization init = overload_resolution_helper<U>::init(func.parameter(i + parameter_offset), args.at(static_cast<size_t>(i)), engine);
      if (init.kind() == Initialization::InvalidInitialization)
      {
        ok = false;
        break;
      }
      current.initializations.push_back(init);
    }

    if (!ok)
      continue;

    details::overloadresolution_process_candidate(current, selected, ambiguous);
  }

  if (ambiguous.function.isNull() && !selected.function.isNull())
    return selected;
  return {};
}

} // namespace script

#endif // LIBSCRIPT_OVERLOAD_RESOLUTION_H
