// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_STACK_H
#define LIBSCRIPT_COMPILER_STACK_H

#include "script/types.h"

namespace script
{

namespace compiler
{

struct Variable
{
  Type type;
  std::string name;
  int index;
  bool global;

  Variable();
  Variable(const Type & t, std::string n, int i, bool g = false);
};

class Stack
{
public:
  Stack() : size(0), capacity(0), data(nullptr) { }
  Stack(const Stack &) = delete;
  Stack(int s);
  ~Stack();

  void clear();

  int addVar(const Type & t, const std::string & name);
  bool exists(const std::string & var) const;
  int indexOf(const std::string & var) const;
  int lastIndexOf(const std::string & var) const;
  void destroy(int n);

  Stack & operator=(const Stack &) = delete;

  const Variable & at(int i) const;
  Variable & operator[](int index);
  const Variable & operator[](int index) const;

protected:
  void realloc(int s);

public:
  int size;
  int capacity;
  Variable *data;
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_STACK_H
