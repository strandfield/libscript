// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TEMPLATE_ARGUMENT_DEDUCTION_H
#define LIBSCRIPT_TEMPLATE_ARGUMENT_DEDUCTION_H

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

class TemplateArgumentDeduction
{
public:
  std::vector<deduction::Deduction> deductions_;
  bool success_;

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

  static TemplateArgumentDeduction process(FunctionTemplate ft, const std::vector<TemplateArgument> & args, const std::vector<Type> & types, const Scope & scp, const std::shared_ptr<ast::TemplateDeclaration> & decl);
  void fill(FunctionTemplate ft, const std::vector<TemplateArgument> & args, const std::vector<Type> & types, const Scope & scp, const std::shared_ptr<ast::TemplateDeclaration> & decl);

private:
  bool process_next_deduction(std::vector<deduction::Deduction>::const_iterator & read, std::vector<deduction::Deduction>::iterator & write, std::vector<deduction::Deduction>::const_iterator end);
};

class TemplateArgumentDeductionEngine
{
private:
  TemplateArgumentDeduction *result_;

  FunctionTemplate template_;
  const std::vector<TemplateArgument> *arguments_;
  const std::vector<Type> *types_;
  Scope scope_;
  std::shared_ptr<ast::FunctionDecl> declaration_;

public:
  TemplateArgumentDeductionEngine(TemplateArgumentDeduction *tad, const FunctionTemplate & ft, const std::vector<TemplateArgument> & result, const std::vector<Type> & types, const Scope & scp, const std::shared_ptr<ast::TemplateDeclaration> & decl);

  inline TemplateArgumentDeduction & result() { return *result_; }

  inline const FunctionTemplate & get_template() const { return template_; }
  inline const std::vector<TemplateArgument> & get_arguments() const { return *arguments_; }
  inline const std::vector<Type> & get_types() const { return *types_; }
  inline const Scope & get_scope() const { return scope_; }

  inline Engine* engine() const { return scope_.engine(); }

  void process();

private:
  void deduce(const ast::FunctionParameter & param, const Type & t);

  void deduce(const ast::QualifiedType & pattern, const Type & input);
  void deduce(const ast::FunctionType & param, const Type & t);

  // deduction from a template argument list
  void deduce(const std::vector<std::shared_ptr<ast::Node>> & pattern, const std::vector<TemplateArgument> & inputs);
  void deduce(const std::shared_ptr<ast::Node> & pattern, const TemplateArgument & input);

  void deduce(const std::shared_ptr<ast::ScopedIdentifier> & pattern, const Type & input);

  void record_deduction(int param_index, const TemplateArgument & value);
};


class TemplatePatternMatching
{
private:
  TemplateArgumentDeduction *deductions_;
  const std::vector<TemplateArgument> *arguments_;
  Scope scope_;
  bool result_;

public:
  TemplatePatternMatching(TemplateArgumentDeduction *tad, const Scope & parameterScope, const std::vector<TemplateArgument> & targs);

  bool match(const std::vector<std::shared_ptr<ast::Node>> & pattern);

  inline TemplateArgumentDeduction & deductions() { return *deductions_; }

  inline const std::vector<TemplateArgument> & arguments() const { return *arguments_; }
  inline const Scope & scope() const { return scope_; }

  inline Engine* engine() const { return scope_.engine(); }

private:
  void fail() { result_ = false; }

private:
  void deduce(const ast::QualifiedType & pattern, const Type & input);
  void deduce(const ast::FunctionType & param, const Type & t);

  // deduction from a template argument list
  void deduce(const std::vector<std::shared_ptr<ast::Node>> & pattern, const std::vector<TemplateArgument> & inputs);
  void deduce(const std::shared_ptr<ast::Node> & pattern, const TemplateArgument & input);

  void deduce(const std::shared_ptr<ast::ScopedIdentifier> & pattern, const Type & input);

  void record_deduction(int param_index, const TemplateArgument & value);
};

} // namespace script

#endif // LIBSCRIPT_TEMPLATE_ARGUMENT_DEDUCTION_H
