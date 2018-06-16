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
  Prototype(const Type & rt, const Type & arg);
  Prototype(const Type & rt, const Type & arg1, const Type & arg2);
  Prototype(const Type & rt, const Type & arg1, const Type & arg2, const Type & arg3);
  Prototype(const Type & rt, const Type & arg1, const Type & arg2, const Type & arg3, const Type & arg4);
  Prototype(const Type & rt, std::initializer_list<Type> && args);

  inline const Type & returnType() const { return mReturnType; }
  void setReturnType(const Type & rt) { mReturnType = rt; }

  int argumentCount() const { return (int) std::distance(mBegin, mEnd); }
  inline int argc() const { return argumentCount(); }
  const Type & argumentType(int index) const;
  inline const Type & argv(int index) const { return argumentType(index); }
  void addArgument(const Type & arg);
  void popArgument();
  std::vector<Type> arguments() const;
  const Type * begin() const;
  const Type * end() const;
  void setParameter(int index, const Type & t);

  bool hasDefaultArgument() const;
  int defaultArgCount() const;

  Prototype & operator=(const Prototype & other);
  bool operator==(const Prototype & other) const;
  bool operator!=(const Prototype & other) const;

private:
  Type mReturnType;
  int mCapacity;
  Type *mBegin;
  Type *mEnd;
  Type mArgs[4];
};

} // namespace script


#endif // LIBSCRIPT_PROTOTYPE_H
