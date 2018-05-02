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
  FunctionTemplate template_;
  std::vector<TemplateArgument> *result_;
  const std::vector<Type> *types_;
  Scope scope_;
  std::vector<deduction::Deduction> deductions_;
  std::shared_ptr<ast::FunctionDecl> declaration_;
  bool success_;

public:

  inline bool success() const { return success_; }
  inline bool failure() const { return !success(); }

  inline const FunctionTemplate & get_template() const { return template_; }
  inline const std::vector<TemplateArgument> & get_arguments() const { return *result_; }
  inline std::vector<TemplateArgument> && move_arguments() const { return std::move(*result_); }
  inline const std::vector<Type> & get_types() const { return *types_; }
  inline const Scope & get_scope() const { return scope_; }
  inline const std::vector<deduction::Deduction> & get_deductions() const { return deductions_; }
  size_t deduction_index(size_t n) const;
  const std::string & deduction_name(size_t n) const;
  const TemplateArgument & deduced_value(size_t n) const;

  inline Engine* engine() const { return scope_.engine(); }

  static TemplateArgumentDeduction process(FunctionTemplate ft, std::vector<TemplateArgument> & result, const std::vector<Type> & types, const Scope & scp, const std::shared_ptr<ast::TemplateDeclaration> & decl);

private:
  void process();
  void deduce(const ast::FunctionParameter & param, const Type & t);

  void deduce(const ast::QualifiedType & pattern, const Type & input);
  void deduce(const ast::FunctionType & param, const Type & t);

  // deduction from a template argument list
  void deduce(const std::vector<std::shared_ptr<ast::Node>> & pattern, const std::vector<TemplateArgument> & inputs);
  void deduce(const std::shared_ptr<ast::Node> & pattern, const TemplateArgument & input);

  void deduce(const std::shared_ptr<ast::ScopedIdentifier> & pattern, const Type & input);

  void record_deduction(int param_index, const TemplateArgument & value);

  void agglomerate_deductions();
  bool process_next_deduction(std::vector<deduction::Deduction>::const_iterator & read, std::vector<deduction::Deduction>::iterator & write, std::vector<deduction::Deduction>::const_iterator end);
  void write_deduced_argument(int index, const TemplateArgument & value);

  void fail();

};

} // namespace script

#endif // LIBSCRIPT_TEMPLATE_ARGUMENT_DEDUCTION_H
