// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_SCOPE_P_H
#define LIBSCRIPT_FUNCTION_SCOPE_P_H

#include "../scope_p.h"

namespace script
{

namespace compiler
{

class FunctionCompiler;

class FunctionScope : public ScopeImpl
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
  ~FunctionScope() = default;

  Engine * engine() const override;
  int kind() const override;

  const std::vector<Class> & classes() const override;
  const std::vector<Enum> & enums() const override;
  const std::vector<Function> & functions() const override;
  const std::vector<LiteralOperator> & literal_operators() const override;
  const std::vector<Namespace> & namespaces() const override;
  const std::vector<Operator> & operators() const override;
  const std::vector<Template> & templates() const override;

  bool lookup(const std::string & name, NameLookupImpl *nl) const override;

  int add_var(const std::string & name, const Type & t, bool global = false);
  void destroy();

  Category category() const;
  bool catch_break() const;
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
