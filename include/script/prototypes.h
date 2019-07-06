// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_PROTOTYPES_H
#define LIBSCRIPT_PROTOTYPES_H

#include "script/prototype.h"

namespace script
{

class LIBSCRIPT_API SingleParameterPrototype : public Prototype
{
public:
  SingleParameterPrototype();
  SingleParameterPrototype(const Type & rt, const Type & param);
  SingleParameterPrototype(const SingleParameterPrototype & other);
  ~SingleParameterPrototype() = default;

private:
  Type mParameter;
};

class LIBSCRIPT_API TwoParametersPrototype : public Prototype
{
public:
  TwoParametersPrototype();
  TwoParametersPrototype(const Type & rt, const Type & p1, const Type & p2);
  TwoParametersPrototype(const TwoParametersPrototype & other);
  ~TwoParametersPrototype() = default;

private:
  Type mParameters[2];
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

class LIBSCRIPT_API DynamicPrototype : public Prototype
{
public:
  DynamicPrototype() = default;
  DynamicPrototype(const DynamicPrototype & other);
  DynamicPrototype(DynamicPrototype && other);
  ~DynamicPrototype() = default;

  DynamicPrototype(const Prototype & other);
  DynamicPrototype(const Type & rt, std::vector<Type> && params);

  void push(const Type & p);
  Type pop();
  void set(size_t i, const Type & p);
  void clear();

  void set(std::vector<Type> && params);

  DynamicPrototype& operator=(const Prototype & other);
  DynamicPrototype& operator=(const DynamicPrototype& other);
  DynamicPrototype& operator=(DynamicPrototype && other);

private:
  std::vector<Type> mParameters;
};

} // namespace script

#endif // LIBSCRIPT_PROTOTYPES_H
