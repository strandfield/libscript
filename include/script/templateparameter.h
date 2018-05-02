// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TEMPLATE_PARAMETER_H
#define LIBSCRIPT_TEMPLATE_PARAMETER_H

#include "script/types.h"

#include "script/ast/forwards.h"

namespace script
{

class LIBSCRIPT_API TemplateParameter
{
public:
  enum Kind {
    TypeTemplateParameter = 1,
    NonTypeTemplateParameter = 2,
    ParameterPackFlag = 8,
  };

  struct TypeParameter {};
  struct ParameterPack {};

  TemplateParameter(TypeParameter, const std::string & n = std::string{});
  TemplateParameter(TypeParameter, ParameterPack, const std::string & n = std::string{});
  TemplateParameter(Type t, const std::string & n = std::string{});
  TemplateParameter(Type t, ParameterPack, const std::string & n = std::string{});

  Kind kind() const;
  bool isPack() const;

  const std::string & name() const;
  void setName(const std::string & name);

  Type type() const; // for non-type parameters

  bool hasDefaultValue() const;
  const std::shared_ptr<ast::Node> & defaultValue() const;
  void setDefaultValue(const std::shared_ptr<ast::Node> & dv);

private:
  char kind_;
  Type type_;
  std::string name_;
  std::shared_ptr<ast::Node> default_value_;
};

} // namespace script

#endif // LIBSCRIPT_TEMPLATE_PARAMETER_H
