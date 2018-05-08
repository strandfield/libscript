// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_SCOPE_P_H
#define LIBSCRIPT_FUNCTION_SCOPE_P_H

#include "script/private/scope_p.h"

namespace script
{

namespace compiler
{

class FunctionCompiler;

class FunctionScope : public ExtensibleScope
{
public:
  enum Category {
    Invalid = 0,
    FunctionArguments = 7,
    FunctionBody = 1,
    IfBody = 2,
    WhileBody = 3,
    ForInit = 4,
    ForBody = 5,
    CompoundStatement = 6,
  };

  FunctionScope(FunctionCompiler *fc, Category cat, Scope p);
  FunctionScope(const FunctionScope & other);
  ~FunctionScope() = default;

  Engine * engine() const override;
  int kind() const override;
  FunctionScope * clone() const override;

  bool lookup(const std::string & name, NameLookupImpl *nl) const override;

  int add_var(const std::string & name, const Type & t);
  void destroy();

  Category category() const;
  bool catch_break() const;
  bool catch_continue() const;
  inline int sp() const { return mSp; }

public:
  Category mCategory;
  FunctionCompiler *mCompiler;
  int mSp;
  int mSize;
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_FUNCTION_SCOPE_P_H
