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
  Prototype(const Prototype &) = delete;
  ~Prototype();

  explicit Prototype(const Type& rt);
  Prototype(const Type& rt, Type* begin, Type* end);

  inline const Type& returnType() const { return mReturnType; }
  void setReturnType(const Type& rt) { mReturnType = rt; }

  size_t parameterCount() const { return static_cast<size_t>(std::distance(mBegin, mEnd)); }
  inline size_t count() const { return parameterCount(); }
  inline size_t size() const { return parameterCount(); }
  inline const Type& parameter(size_t index) const { return *(mBegin + index); }
  inline const Type& at(size_t index) const { return parameter(index); }
  inline std::vector<Type> parameters() const { return std::vector<Type>{ mBegin, mEnd }; }
  inline const Type* begin() const { return mBegin; }
  inline const Type* end() const { return mEnd; }
  inline void setParameter(size_t index, const Type& t) { *(mBegin + index) = t; }

  Prototype& operator=(const Prototype&) = delete;
  inline Type& operator[](size_t index) { return *(mBegin + index); }
  bool operator==(const Prototype& other) const;

protected:
  inline void setParameters(Type *begin, Type *end) { mBegin = begin; mEnd = end; }

private:
  Type mReturnType;
  Type *mBegin;
  Type *mEnd;
};

inline bool operator!=(const Prototype& lhs, const Prototype& rhs) { return !(lhs == rhs); }

} // namespace script

#endif // LIBSCRIPT_PROTOTYPE_H
