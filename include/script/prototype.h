// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_PROTOTYPE_H
#define LIBSCRIPT_PROTOTYPE_H

#include <vector>

#include "script/types.h"

namespace script
{

class LIBSCRIPT_API Prototype
{
public:
  Prototype();
  Prototype(const Prototype & other);
  Prototype(Prototype && other);
  ~Prototype();

  explicit Prototype(const Type & rt);
  Prototype(const Type & rt, const Type & p);
  Prototype(const Type & rt, const Type & p1, const Type & p2);
  Prototype(const Type & rt, const Type & p1, const Type & p2, const Type & p3);
  Prototype(const Type & rt, const Type & p1, const Type & p2, const Type & p3, const Type & p4);
  Prototype(const Type & rt, std::initializer_list<Type> && params);

  inline const Type & returnType() const { return mReturnType; }
  void setReturnType(const Type & rt) { mReturnType = rt; }

  int parameterCount() const { return (int) std::distance(mBegin, mEnd); }
  inline int count() const { return parameterCount(); }
  inline int size() const { return parameterCount(); }
  inline const Type & parameter(int index) const { return *(mBegin + index); }
  inline const Type & at(int index) const { return parameter(index); }
  void addParameter(const Type & param);
  void popParameter();
  inline std::vector<Type> parameters() const { return std::vector<Type>{ mBegin, mEnd }; }
  inline const Type * begin() const { return mBegin; }
  inline const Type * end() const { return mEnd; }
  inline void setParameter(int index, const Type & t) { *(mBegin + index) = t; }

  Prototype & operator=(const Prototype & other);
  inline Type & operator[](int index) { return *(mBegin + index); }
  bool operator==(const Prototype & other) const;
  inline bool operator!=(const Prototype & other) const { return !operator==(other); }

private:
  Type mReturnType;
  int mCapacity;
  Type *mBegin;
  Type *mEnd;
  Type mParams[4];
};

} // namespace script


#endif // LIBSCRIPT_PROTOTYPE_H
