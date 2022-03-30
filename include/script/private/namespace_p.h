// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_NAMESPACE_P_H
#define LIBSCRIPT_NAMESPACE_P_H

#include "script/private/symbol_p.h"

#include "script/enum.h"
#include "script/class.h"
#include "script/literals.h"
#include "script/namespace.h"
#include "script/operator.h"
#include "script/template.h"
#include "script/value.h"
#include "script/typedefs.h"

#include <map>
#include <vector>

namespace script
{

class ModuleInterface;

class NamespaceImpl : public SymbolImpl
{
public:
  std::string name;
  std::map<std::string, Value> variables;
  std::vector<Enum> enums;
  std::vector<Class> classes;
  std::vector<Function> functions;
  std::vector<Namespace> namespaces;
  std::vector<Operator> operators;
  std::vector<LiteralOperator> literal_operators;
  std::vector<Template> templates;
  std::vector<Typedef> typedefs;
  std::weak_ptr<ModuleInterface> the_module;

public:
  NamespaceImpl(const std::string & n, Engine *e)
    : SymbolImpl(e)
    , name(n)
  {

  }
  NamespaceImpl(const NamespaceImpl &) = delete;
  virtual ~NamespaceImpl() = default;

  Name get_name() const override;
};

} // namespace script


#endif // LIBSCRIPT_NAMESPACE_P_H
