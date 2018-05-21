// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_TEMPLATE_SPECIALIZATION_H
#define LIBSCRIPT_COMPILER_TEMPLATE_SPECIALIZATION_H

#include "libscriptdefs.h"

#include "script/private/scope_p.h"

#include "script/templateparameter.h"

#include "script/compiler/templatedefinition.h"

#include <vector>

namespace script
{

class ClassTemplate;
class FunctionTemplate;
class PartialTemplateSpecialization;
class TemplateArgument;

namespace compiler
{

class LIBSCRIPT_API TemplatePartialOrdering
{
public:
  enum ComparisonResult {
    NotComparable = 0,
    Indistinguishable = 1,
    FirstIsMoreSpecialized = 2,
    SecondIsMoreSpecialized = 3,
  };
public:
  ComparisonResult value_;

  TemplatePartialOrdering() : value_(Indistinguishable) {}
  TemplatePartialOrdering(const TemplatePartialOrdering &) = default;
  ~TemplatePartialOrdering() = default;
  TemplatePartialOrdering(ComparisonResult r) : value_(r) {}

  inline bool positive() const { return value_ == FirstIsMoreSpecialized || value_ == SecondIsMoreSpecialized; }

  TemplatePartialOrdering & operator=(const TemplatePartialOrdering &) = default;
};

inline bool operator==(const TemplatePartialOrdering & lhs, const TemplatePartialOrdering & rhs)
{
  return lhs.value_ == rhs.value_;
}

inline bool operator!=(const TemplatePartialOrdering & lhs, const TemplatePartialOrdering & rhs)
{
  return lhs.value_ != rhs.value_;
}

TemplatePartialOrdering operator&(const TemplatePartialOrdering & lhs, const TemplatePartialOrdering & rhs);


class LIBSCRIPT_API TemplateSpecialization
{
public:
  // compares two user-defined FunctionTemplate
  static TemplatePartialOrdering compare(const FunctionTemplate & a, const FunctionTemplate & b);

  template<typename T>
  struct Scoped
  {
    Scope scope;
    T value;
  };

  typedef Scoped<std::shared_ptr<ast::VariableDecl>> SVarDecl;
  typedef Scoped<ast::QualifiedType> SType;
  typedef Scoped<ast::FunctionType> SFunctionType;
  typedef Scoped<std::shared_ptr<ast::Node>> STemplateArg;


private:
  static TemplatePartialOrdering compare(const SVarDecl & a, const SVarDecl & b);
  static TemplatePartialOrdering compare(const SType & a, const SType & b);
  static TemplatePartialOrdering compare(const SFunctionType & a, const SFunctionType & b);
  static TemplatePartialOrdering compare_from_args(const Scope & scpa, const std::vector<std::shared_ptr<ast::Node>> & a, const Scope & scpb, const std::vector<std::shared_ptr<ast::Node>> & b);
  static TemplatePartialOrdering compare_targ(const STemplateArg & a, const STemplateArg & b);
  static TemplatePartialOrdering compare_from_qual(const ast::QualifiedType & a, const ast::QualifiedType & b);

public:
  // compares two patterns of class template specialization specialization 
  static TemplatePartialOrdering compare(const PartialTemplateSpecialization & a, const PartialTemplateSpecialization & b);
};

class LIBSCRIPT_API TemplateSpecializationSelector
{
public:
  
  std::pair<PartialTemplateSpecialization, std::vector<TemplateArgument>> select(const ClassTemplate & ct, const std::vector<TemplateArgument> & targs);
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_TEMPLATE_SPECIALIZATION_H
