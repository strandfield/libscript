// Copyright (C) 2018-2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_SYMBOL_H
#define LIBSCRIPT_SYMBOL_H

#include "libscriptdefs.h"
#include "script/operators.h"
#include "script/types.h"

namespace script
{

class Attributes;
class Class;
class ClassBuilder;
class ClassTemplateBuilder;
class Engine;
class EnumBuilder;
class FunctionBuilder;
class FunctionTemplateBuilder;
class Name;
class Namespace;
class OperatorBuilder;
class Script;
class SymbolImpl;
class TypedefBuilder;

/*!
 * \class Symbol
 * \brief provides storage for any symbol (class, namespace, etc...)
 */

class LIBSCRIPT_API Symbol
{
public:
  Symbol() = default;
  Symbol(const Symbol &) = default;
  ~Symbol() = default;

  explicit Symbol(const script::Class & c);
  explicit Symbol(const Namespace & n);

  explicit Symbol(const std::shared_ptr<SymbolImpl> & impl);

  inline bool isNull() const { return d == nullptr; }

  Engine* engine() const;

  bool isClass() const;
  script::Class toClass() const;

  bool isNamespace() const;
  Namespace toNamespace() const;

  Name name() const;
  Symbol parent() const;
  Script script() const;

  Attributes attributes() const;

  ClassBuilder newClass(const std::string & name);
  ClassBuilder newClass(std::string && name);
  ClassTemplateBuilder newClassTemplate(const std::string & name);
  ClassTemplateBuilder newClassTemplate(std::string && name);
  EnumBuilder newEnum(std::string && name);
  FunctionBuilder newFunction(const std::string & name);
  FunctionTemplateBuilder newFunctionTemplate(const std::string & name);
  FunctionTemplateBuilder newFunctionTemplate(std::string && name);
  OperatorBuilder newOperator(OperatorName op);
  TypedefBuilder newTypedef(const Type & t, const std::string & name);
  TypedefBuilder newTypedef(const Type & t, std::string && name);

  Symbol & operator=(const Symbol &) = default;

  inline const std::shared_ptr<SymbolImpl> & impl() const { return d; }

private:
  std::shared_ptr<SymbolImpl> d;
};

/*!
 * \endclass
 */

} // namespace script

#endif // LIBSCRIPT_SYMBOL_H
