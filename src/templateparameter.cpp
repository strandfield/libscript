// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/templateparameter.h"

namespace script
{

TemplateParameter::TemplateParameter(TypeParameter, const std::string & n)
  : kind_(TypeTemplateParameter), name_(n) { }

TemplateParameter::TemplateParameter(TypeParameter, ParameterPack, const std::string & n)
  : kind_(TypeTemplateParameter | ParameterPackFlag), name_(n) { }

TemplateParameter::TemplateParameter(Type t, const std::string & n)
  : kind_(NonTypeTemplateParameter), type_(t), name_(n) { }

TemplateParameter::TemplateParameter(Type t, ParameterPack, const std::string & n)
  : kind_(NonTypeTemplateParameter | ParameterPackFlag), type_(t), name_(n) { }

TemplateParameter::Kind TemplateParameter::kind() const
{
  return static_cast<Kind>(kind_ & 3);
}

bool TemplateParameter::isPack() const
{
  return kind_ & ParameterPackFlag;
}

const std::string & TemplateParameter::name() const
{
  return name_;
}

void TemplateParameter::setName(const std::string & name)
{
  name_ = name;
}

Type TemplateParameter::type() const
{
  return type_;
}

bool TemplateParameter::hasDefaultValue() const
{
  return default_value_ != nullptr;
}

const std::shared_ptr<ast::Node> & TemplateParameter::defaultValue() const
{
  return default_value_;
}

void TemplateParameter::setDefaultValue(const std::shared_ptr<ast::Node> & dv)
{
  default_value_ = dv;
}

} // namespace script
