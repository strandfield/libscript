// Copyright (C) 2018-2022 Vincent Chambrin
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
class Attributes;
class Cast;
class Class;
class DefaultArguments;
class Engine;
namespace program { class Expression; class Statement; }
class FunctionImpl;
class FunctionBodyInterface;
class FunctionTemplate;
class LiteralOperator;
class Locals;
class Name;
class Namespace;
class Operator;
class Script;
class TemplateArgument;
class UserData;

/*!
 * \class Function
 * \brief represents a function
 */

class LIBSCRIPT_API Function
{
public:
  Function() = default;
  Function(const Function & other) = default;
  Function(Function&&) noexcept = default;
  ~Function() = default;

  explicit Function(const std::shared_ptr<FunctionImpl> & impl);

  bool isNull() const;

  const std::string & name() const;
  Name getName() const;

  const Prototype& prototype() const;
  const Type& parameter(size_t index) const;
  const Type& returnType() const;

  DefaultArguments defaultArguments() const;

  Script script() const;

  Attributes attributes() const;

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

  std::shared_ptr<program::Statement> program() const;

  std::shared_ptr<UserData> data() const;

  Engine* engine() const;
  inline const std::shared_ptr<FunctionImpl> & impl() const { return d; }

  Value call(Locals& locals) const;

  Value invoke(std::initializer_list<Value>&& args) const;
  Value invoke(const std::vector<Value>& args) const;
  Value invoke(const Value* begin, const Value* end) const;

  Function& operator=(const Function&) = default;
  Function& operator=(Function&&) noexcept = default;
  bool operator==(const Function & other) const;
  bool operator!=(const Function & other) const;

protected:
  std::shared_ptr<FunctionImpl> d;
};

/*!
 * \endclass
 */

} // namespace script

#endif // LIBSCRIPT_FUNCTION_H
