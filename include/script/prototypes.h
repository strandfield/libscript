// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_PROTOTYPES_H
#define LIBSCRIPT_PROTOTYPES_H

#include "script/prototype.h"

#include <algorithm>
#include <array>
#include <vector>

namespace script
{

template<size_t N>
class FixedSizePrototype : public Prototype
{
public:
  FixedSizePrototype(Type rt = Type::Void)
    : Prototype(rt)
  {
    setParameters(m_parameters.data(), m_parameters.data() + N);
  }

  FixedSizePrototype(const Prototype& other) 
    : Prototype(other.returnType())
  {
    assert(other.size() <= N);
    std::copy(other.begin(), other.end(), m_parameters.begin());
    setParameters(m_parameters.data(), m_parameters.data() + other.size());
  }

  FixedSizePrototype(const FixedSizePrototype<N>& other)
    : Prototype(other.returnType()),
      m_parameters(other.m_parameters)
  {
    setParameters(m_parameters.data(), m_parameters.data() + N);
  }

  ~FixedSizePrototype() = default;

  FixedSizePrototype<N>& operator=(const FixedSizePrototype<N>&) = default;

protected:
  std::array<Type, N> m_parameters;
};

class LIBSCRIPT_API SingleParameterPrototype : public FixedSizePrototype<1>
{
public:
  SingleParameterPrototype() = default;
  SingleParameterPrototype(const Type& rt, const Type& param);
  SingleParameterPrototype(const SingleParameterPrototype&) = default;
  ~SingleParameterPrototype() = default;
};

class LIBSCRIPT_API TwoParametersPrototype : public FixedSizePrototype<2>
{
public:
  TwoParametersPrototype() = default;
  TwoParametersPrototype(const Type& rt, const Type& p1, const Type& p2);
  TwoParametersPrototype(const TwoParametersPrototype&) = default;
  ~TwoParametersPrototype() = default;
};

class LIBSCRIPT_API CastPrototype : public SingleParameterPrototype
{
public:
  using SingleParameterPrototype::SingleParameterPrototype;
};

class LIBSCRIPT_API DestructorPrototype : public SingleParameterPrototype
{
public:
  using SingleParameterPrototype::SingleParameterPrototype;
};

class LIBSCRIPT_API UnaryOperatorPrototype : public SingleParameterPrototype
{
public:
  using SingleParameterPrototype::SingleParameterPrototype;
};

class LIBSCRIPT_API BinaryOperatorPrototype : public TwoParametersPrototype
{
public:
  using TwoParametersPrototype::TwoParametersPrototype;
};

/*!
 * \class DynamicPrototype
 * \brief a prototype with an arbitrary number of parameters
 */

class LIBSCRIPT_API DynamicPrototype : public Prototype
{
public:
  DynamicPrototype() = default;
  DynamicPrototype(const DynamicPrototype & other);
  DynamicPrototype(DynamicPrototype && other);
  ~DynamicPrototype() = default;

  DynamicPrototype(const Prototype & other);
  DynamicPrototype(const Type & rt, std::vector<Type> params);

  void push(const Type & p);
  Type pop();
  void set(size_t i, const Type & p);
  void clear();

  void set(std::vector<Type> params);

  DynamicPrototype& operator=(const Prototype & other);
  DynamicPrototype& operator=(const DynamicPrototype& other);
  DynamicPrototype& operator=(DynamicPrototype && other);

private:
  std::vector<Type> mParameters;
};

/*!
 * \endclass
 */

} // namespace script

#endif // LIBSCRIPT_PROTOTYPES_H
