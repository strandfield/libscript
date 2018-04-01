// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_NAMELOOKUP_H
#define LIBSCRIPT_NAMELOOKUP_H

#include "scope.h"

#include "class.h"
#include "enum.h"
#include "enumvalue.h"
#include "namespace.h"
#include "script.h"
#include "template.h"

/// TODO : remove this, replace it by a simple forward declaration
#include "ast/ast.h"

namespace script
{

namespace program
{
class Expression;
}

namespace compiler
{
class AbstractExpressionCompiler;
} // namespace compiler

struct NameLookupImpl;


class LIBSCRIPT_API NameLookup
{
public:
  NameLookup() = default;
  NameLookup(const NameLookup &) = default;
  ~NameLookup() = default;

  NameLookup(const std::shared_ptr<NameLookupImpl> & impl);

  inline bool isNull() const { return d == nullptr; }

  const Scope & scope() const;
  const std::string & name() const;

  enum ResultType {
    UnknownName,
    FunctionName,
    TemplateName,
    TypeName,
    VariableName,
    DataMemberName, 
    GlobalName, 
    LocalName,
    EnumValueName,
    NamespaceName,
    CaptureName,
  };

  ResultType resultType() const;

  const std::vector<Function> & functions() const;
  const Type & typeResult() const;
  const Value & variable() const;
  const Template & templateResult() const;
  int captureIndex() const;
  int dataMemberIndex() const;
  int globalIndex() const;
  int localIndex() const;
  const EnumValue & enumValueResult() const;
  const Namespace & namespaceResult() const;

  static NameLookup resolve(const std::shared_ptr<ast::Identifier> & name, const Scope & scope);
  static NameLookup resolve(const std::string & name, const Scope & scope);
  static NameLookup resolve(Operator::BuiltInOperator op, const Scope & scope);

  static NameLookup resolve(const std::shared_ptr<ast::Identifier> & name, compiler::AbstractExpressionCompiler *compiler);
  static NameLookup resolve(const std::shared_ptr<ast::Identifier> & name, const std::vector<std::shared_ptr<program::Expression>> & args, compiler::AbstractExpressionCompiler *compiler);

  static NameLookup member(const std::string & name, const Class & cla);

  NameLookup & operator=(const NameLookup &) = default;

  inline std::shared_ptr<NameLookupImpl> impl() const { return d; }

protected:
  std::shared_ptr<NameLookupImpl> d;
};

} // namespace script

#endif // LIBSCRIPT_NAMELOOKUP_H
