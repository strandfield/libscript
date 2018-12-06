// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TEMPLATE_ARGUMENT_DEDUCTION_H
#define LIBSCRIPT_TEMPLATE_ARGUMENT_DEDUCTION_H

/// TODO: try refactor TemplateArgumentDeduction and TemplatePatternMatching

#include "script/functiontemplate.h"

#include "script/scope.h"

#include "script/ast/forwards.h"

namespace script
{

namespace deduction
{

struct Deduction
{
  size_t param_index;
  TemplateArgument deduced_value;
};

} // namespace deduction

class LIBSCRIPT_API TemplateArgumentDeduction
{
public:
  std::vector<deduction::Deduction> deductions_;
  bool success_;
  /// TODO: add failure message

public:
  TemplateArgumentDeduction();
  TemplateArgumentDeduction(const TemplateArgumentDeduction &) = default;
  ~TemplateArgumentDeduction() = default;
  
  inline bool success() const { return success_; }
  inline bool failure() const { return !success(); }

  void fail();
  inline void reset_success_flag() { success_ = true; }
  inline void set_success(bool b) { success_ = b; }

  inline const std::vector<deduction::Deduction> & get_deductions() const { return deductions_; }
  size_t deduction_index(size_t n) const;
  const TemplateArgument & deduced_value(size_t n) const;
  bool has_deduction_for(size_t param_index) const;
  const TemplateArgument & deduced_value_for(size_t param_index) const;

  void record_deduction(int param_index, const TemplateArgument & value);
  void agglomerate_deductions();

  static TemplateArgumentDeduction process(FunctionTemplate ft, const std::vector<TemplateArgument> & args, const std::vector<Type> & types, const std::shared_ptr<ast::TemplateDeclaration> & decl);
  void fill(FunctionTemplate ft, const std::vector<TemplateArgument> & args, const std::vector<Type> & types, const std::shared_ptr<ast::TemplateDeclaration> & decl);

private:
  bool process_next_deduction(std::vector<deduction::Deduction>::const_iterator & read, std::vector<deduction::Deduction>::iterator & write, std::vector<deduction::Deduction>::const_iterator end);
};

class TemplatePatternMatching
{
private:
  TemplateArgumentDeduction *deductions_;
  Scope scope_;
  bool result_;

public:
  TemplatePatternMatching(TemplateArgumentDeduction *tad, const Scope & patternScope);

  bool match(const std::vector<std::shared_ptr<ast::Node>> & pattern, const std::vector<TemplateArgument> & inputs);
  bool match(const std::shared_ptr<ast::FunctionDecl> & pattern, const Prototype & input);

  inline TemplateArgumentDeduction & deductions() { return *deductions_; }

  inline const Scope & scope() const { return scope_; }

  inline Engine* engine() const { return scope_.engine(); }

private:
  void fail() { result_ = false; }

private:
  void match_(const ast::QualifiedType & pattern, const Type & input);
  void match_(const ast::FunctionType & param, const Type & t);

  // deduction from a template argument list
  void match_(const std::vector<std::shared_ptr<ast::Node>> & pattern, const std::vector<TemplateArgument> & inputs);
  void match_(const std::shared_ptr<ast::Node> & pattern, const TemplateArgument & input);

  void match_(const std::shared_ptr<ast::ScopedIdentifier> & pattern, const Type & input);

  void record_deduction(int param_index, const TemplateArgument & value);
};

} // namespace script

#endif // LIBSCRIPT_TEMPLATE_ARGUMENT_DEDUCTION_H
