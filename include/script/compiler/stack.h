// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_STACK_H
#define LIBSCRIPT_COMPILER_STACK_H

#include "script/types.h"

#include "script/utils/stringview.h"

#include <vector>

namespace script
{

namespace compiler
{

struct Variable
{
  Type type;
  utils::StringView name;
  size_t index;
  bool global;
  bool is_static;

  Variable();
  Variable(const Type & t, utils::StringView n, size_t i, bool g = false, bool s = false);
};

class Stack
{
public:
  std::vector<Variable> data;

public:
  Stack() = default;
  Stack(const Stack &) = delete;

  size_t size() const { return data.size(); }
  void clear() { data.clear(); }

  int addVar(const Type & t, utils::StringView name);
  int indexOf(const std::string & var) const;
  int lastIndexOf(const std::string & var) const;
  int lastIndexOf(utils::StringView var) const;
  void destroy(size_t n);

  Stack & operator=(const Stack &) = delete;

  const Variable& at(size_t i) const { return data.at(i); }
  Variable& operator[](size_t index) { return data[index]; }
  const Variable& operator[](size_t index) const { return data[index]; }
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_STACK_H
