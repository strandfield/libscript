// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_SYMBOL_H
#define LIBSCRIPT_SYMBOL_H

#include "libscriptdefs.h"
#include "script/operators.h"
#include "script/types.h"

namespace script
{

class Class;
class ClassTemplateBuilder;
class Engine;
class FunctionBuilder;
class FunctionTemplateBuilder;
class Namespace;
class SymbolImpl;
class TypedefBuilder;

class LIBSCRIPT_API Symbol
{
public:
  Symbol() = default;
  Symbol(const Symbol &) = default;
  ~Symbol() = default;

  explicit Symbol(const Class & c);
  explicit Symbol(const Namespace & n);

  explicit Symbol(const std::shared_ptr<SymbolImpl> & impl);

  inline bool isNull() const { return d == nullptr; }

  Engine* engine() const;

  bool isClass() const;
  Class toClass() const;

  bool isNamespace() const;
  Namespace toNamespace() const;

  ClassTemplateBuilder ClassTemplate(const std::string & name);
  ClassTemplateBuilder ClassTemplate(std::string && name);
  FunctionBuilder Function(const std::string & name);
  FunctionTemplateBuilder FunctionTemplate(const std::string & name);
  FunctionTemplateBuilder FunctionTemplate(std::string && name);
  FunctionBuilder Operation(OperatorName op);
  TypedefBuilder Typedef(const Type & t, const std::string & name);
  TypedefBuilder Typedef(const Type & t, std::string && name);

  Symbol & operator=(const Symbol &) = default;

  inline const std::shared_ptr<SymbolImpl> & impl() const { return d; }

private:
  std::shared_ptr<SymbolImpl> d;
};



class LIBSCRIPT_API TypedefBuilder
{
public:
  Symbol symbol_;
  std::string name_;
  Type type_;
public:
  TypedefBuilder(const Symbol & s, const std::string & name, const Type & type) 
    : symbol_(s), name_(name), type_(type) {}

  TypedefBuilder(const Symbol & s, std::string && name, const Type & type)
    : symbol_(s), name_(std::move(name)), type_(type) {}

  TypedefBuilder(const TypedefBuilder &) = default;
  ~TypedefBuilder() = default;

  void create();
};

} // namespace script

#endif // LIBSCRIPT_SYMBOL_H
