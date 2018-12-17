// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_H
#define LIBSCRIPT_FUNCTION_H

#include "script/callbacks.h"
#include "script/prototype.h"

#include <string>

namespace script
{

enum class AccessSpecifier;
class Cast;
class Class;
class Engine;
namespace program { class Expression; class Statement; }
class FunctionImpl;
class FunctionTemplate;
class LiteralOperator;
class Name;
class Namespace;
class Operator;
class Script;
class TemplateArgument;
class UserData;

class LIBSCRIPT_API Function
{
public:
  Function() = default;
  Function(const Function & other) = default;
  ~Function() = default;

  explicit Function(const std::shared_ptr<FunctionImpl> & impl);

  bool isNull() const;

  const std::string & name() const;
  Name getName() const;

  const Prototype & prototype() const;
  const Type & parameter(int index) const;
  const Type & returnType() const;

  [[deprecated("Use defaultArguments().size() != 0 instead")]]
  bool hasDefaultArguments() const;
  [[deprecated("Use defaultArguments().size() instead")]]
  size_t defaultArgumentCount() const;
  [[deprecated("default arguments should be processed before the Function object is constructed")]]
  void addDefaultArgument(const std::shared_ptr<program::Expression> & value);
  [[deprecated("default arguments should be processed before the Function object is constructed")]]
  void addDefaultArgument(const script::Value & val, ParameterPolicy policy = ParameterPolicy::Copy);
  const std::vector<std::shared_ptr<program::Expression>> & defaultArguments() const;

  Script script() const;

  bool isConstructor() const;
  bool isDefaultConstructor() const;
  bool isCopyConstructor() const;
  bool isMoveConstructor() const;
  bool isDestructor() const;

  bool isNative() const;
  bool isExplicit() const;
  bool isConst() const;
  bool isVirtual() const;
  bool isPureVirtual() const;
  bool isDefaulted() const;
  bool isDeleted() const;

  bool isMemberFunction() const;
  bool isStatic() const;
  bool isSpecial() const;
  inline bool isNonStaticMemberFunction() const { return isMemberFunction() && !isStatic(); }
  bool hasImplicitObject() const;
  Class memberOf() const;
  AccessSpecifier accessibility() const;
  Namespace enclosingNamespace() const;

  bool isOperator() const;
  Operator toOperator() const;

  bool isLiteralOperator() const;
  LiteralOperator toLiteralOperator() const;

  bool isCast() const;
  Cast toCast() const;

  bool isTemplateInstance() const;
  FunctionTemplate instanceOf() const;
  const std::vector<TemplateArgument> & arguments() const;

  NativeFunctionSignature native_callback() const;
  std::shared_ptr<program::Statement> program() const;

  const std::shared_ptr<UserData> & data() const;

  Engine * engine() const;
  inline const std::shared_ptr<FunctionImpl> & impl() const { return d; }

  Function & operator=(const Function & other);
  bool operator==(const Function & other) const;
  bool operator!=(const Function & other) const;

protected:
  std::shared_ptr<FunctionImpl> d;
};


} // namespace script

#endif // LIBSCRIPT_FUNCTION_H
