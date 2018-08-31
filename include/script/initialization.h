// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_INITIALIZATION_H
#define LIBSCRIPT_INITIALIZATION_H

#include "libscriptdefs.h"

#include "script/conversions.h"

namespace script
{

namespace program
{
class Expression;
} // namespace program

struct InitializationData;

class LIBSCRIPT_API Initialization
{
public:

  enum Category {
    InvalidInitialization = 0,
    ValueInitialization,
    DefaultInitialization = ValueInitialization,
    DirectInitialization,
    CopyInitialization,
    ReferenceInitialization,
    ListInitialization,
    AggregateInitialization,
  };

  Initialization();
  Initialization(const Initialization &) = default;
  ~Initialization() = default;

  Initialization(Category cat);
  Initialization(Category cat, Type t);
  Initialization(Category cat, Function ctor);
  Initialization(Category cat, const Conversion & conv);

  inline Category kind() const { return mCategory; }
  bool isValid() const;

  inline bool isReferenceInitialization() const { return mCategory == ReferenceInitialization; }
  bool createsTemporary() const;

  const Conversion & conversion() const { return mConversion; }

  ConversionRank rank() const;

  bool hasInitializations() const;
  std::vector<Initialization> & initializations();
  const std::vector<Initialization> & initializations() const;
  Type destType() const;
  const Function & constructor() const;

  static int comp(const Initialization & a, const Initialization & b);

  static Initialization compute(const Type & vartype, Engine *engine);
  static Initialization compute(const Type & vartype, const Type & arg, Engine *engine, Category cat = CopyInitialization);
  static Initialization compute(const Type & vartype, const std::shared_ptr<program::Expression> & expr, Engine *engine);

private:
  Category mCategory;
  Conversion mConversion;
  std::shared_ptr<InitializationData> mData;
};

} // namespace script


#endif // LIBSCRIPT_INITIALIZATION_H
