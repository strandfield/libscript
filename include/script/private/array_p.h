// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_ARRAY_P_H
#define LIBSCRIPT_ARRAY_P_H

#include "script/types.h"
#include "script/function.h"
#include "script/userdata.h"

namespace script
{

class ClassTemplate;

struct ArrayData
{
  Type typeId;
  Type elementType;
  Function constructor; // elements default constructor
  Function copyConstructor; // elements copy constructor
  Function destructor; // elements destructor
};

class SharedArrayData : public UserData
{
public:
  SharedArrayData(const ArrayData & d);
  ~SharedArrayData() = default;
  ArrayData data;
};

class LIBSCRIPT_API ArrayImpl
{
public:
  ArrayImpl();
  ArrayImpl(const ArrayData & d, Engine *e);
  ~ArrayImpl();

  ArrayImpl * copy() const;

  void destroy();
  void allocate(int n);
  void resize(int s);
  void assign(const ArrayImpl & other);

  static ClassTemplate register_array_template(Engine *e);

  /// TODO: use shared array data instead ?
  ArrayData data;
  int size;
  Value *elements;
  Engine *engine;
};

} // namespace script

#endif // LIBSCRIPT_ARRAY_P_H
