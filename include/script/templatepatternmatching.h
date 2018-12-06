// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TEMPLATE_PATTERN_MATCHING_H
#define LIBSCRIPT_TEMPLATE_PATTERN_MATCHING_H

#include "script/templateargumentdeduction.h"



namespace script
{

class LIBSCRIPT_API TemplatePatternMatching2
{
public:

  enum Mode {
    PM_Deduction,
    PM_TemplateSelection,
  };

private:
  TemplateArgumentDeduction *deductions_;
  const std::vector<TemplateArgument> emptyargs_;
  const std::vector<TemplateArgument> *arguments_;
  Template template_;
  Scope scope_;
  Mode working_mode_;

public:
  TemplatePatternMatching2(const Template & tmplt, TemplateArgumentDeduction *tad);

  bool match(const std::vector<std::shared_ptr<ast::Node>> & pattern, const std::vector<TemplateArgument> & inputs);
  bool match(const std::shared_ptr<ast::FunctionDecl> & pattern, const Prototype & input);

  void deduce(const std::shared_ptr<ast::FunctionDecl> & pattern, const std::vector<Type> & inputs);

  inline TemplateArgumentDeduction & deductions() { return *deductions_; }

  inline const Template & getTemplate() const { return template_; }

  inline const Scope & scope() const { return scope_; }
  inline void setScope(const Scope & scp) { scope_ = scp; }

  const std::vector<TemplateArgument> & arguments() const { return *arguments_; }
  void setArguments(const std::vector<TemplateArgument> & targs) { arguments_ = &targs; }

  inline Engine* engine() const { return template_.engine(); }

private:
  void deduce(const ast::FunctionParameter & param, const Type & t);

  bool match_(const ast::QualifiedType & pattern, const Type & input);
  bool match_(const ast::FunctionType & param, const Type & t);

  // deduction from a template argument list
  bool match_(const std::vector<std::shared_ptr<ast::Node>> & pattern, const std::vector<TemplateArgument> & inputs);
  bool match_(const std::shared_ptr<ast::Node> & pattern, const TemplateArgument & input);

  bool match_(const std::shared_ptr<ast::ScopedIdentifier> & pattern, const Type & input);

  void record_deduction(int param_index, const TemplateArgument & value);
};

} // namespace script

#endif // LIBSCRIPT_TEMPLATE_PATTERN_MATCHING_H
