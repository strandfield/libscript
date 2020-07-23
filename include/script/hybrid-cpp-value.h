// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_HYBRIDCPPVALUE_H
#define LIBSCRIPT_HYBRIDCPPVALUE_H

#include "script/value.h"

#include <vector>

namespace script
{

template<typename T>
class HybridCppValue : public IValue
{
public:
  T value;
  std::vector<Value> members;

public:
  ~HybridCppValue() = default;

  HybridCppValue(script::Engine* e, T val)
    : IValue(script::Type::make<T>(), e),
      value(std::move(val))
  {

  }

  HybridCppValue(script::Engine* e, script::Type t, T val)
    : IValue(t, e),
      value(std::move(val))
  {

  }

  void* ptr() override { return &value; }

  size_t size() const override { return members.size(); }
  void push(const Value& val) override { members.push_back(val); }
  Value pop() override { Value back = members.back();  members.pop_back(); return back; }
  Value& at(size_t index) override { return members.at(index); }
};

} // namespace script

#endif // LIBSCRIPT_HYBRIDCPPVALUE_H
