// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_CLASS_H
#define LIBSCRIPT_CLASS_H

#include "script/accessspecifier.h"
#include "script/callbacks.h"
#include "script/operators.h"

#include <map>
#include <vector>

namespace script
{

class Cast;
class ClassBuilder;
class ClassImpl;
class ClassTemplate;
class ClosureType;
class DataMember;
class Engine;
class Enum;
class EnumBuilder;
class Function;
class FunctionBuilder;
class Namespace;
class Operator;
class Script;
class StaticDataMember;
class Template;
class TemplateArgument;
class Type;
class Typedef;
class UserData;
class Value;

class LIBSCRIPT_API Class
{
public:
  Class() = default;
  Class(const Class & other) = default;
  ~Class() = default;
  
  explicit Class(const std::shared_ptr<ClassImpl> & impl);

  bool isNull() const;
  int id() const;
  
  const std::string & name() const;

  Class parent() const;
  bool inherits(const Class & type) const;
  int inheritanceLevel(const Class & type) const;
  bool isFinal() const;

  bool isClosure() const;
  ClosureType toClosure() const;

  typedef script::DataMember DataMember;

  const std::vector<DataMember> & dataMembers() const;
  /// TODO : add cumulatedDataMembers()
  // std::vector<DataMember> cumulatedDataMembers() const;
  int cumulatedDataMemberCount() const;
  int attributesOffset() const;
  int attributeIndex(const std::string & attrName) const;


  Script script() const;

  const std::shared_ptr<UserData> & data() const;

  Value instantiate(const std::vector<Value> & args);

  const std::vector<Class> & classes() const;

  EnumBuilder Enum(const std::string & name);
  const std::vector<script::Enum> & enums() const;

  const std::vector<Template> & templates() const;
  const std::vector<Typedef> & typedefs() const;

  const std::vector<Operator> & operators() const;
  const std::vector<Cast> & casts() const;

  const std::vector<Function> & constructors() const;
  Function defaultConstructor() const;
  bool isDefaultConstructible() const;
  Function copyConstructor() const;
  Function moveConstructor() const;
  bool isCopyConstructible() const;
  bool isMoveConstructible() const;

  [[deprecated("Use Destructor() instead")]] Function newDestructor(NativeFunctionSignature func);
  Function destructor() const;
  
  FunctionBuilder Constructor(NativeFunctionSignature func = nullptr) const;
  FunctionBuilder Destructor(NativeFunctionSignature func = nullptr) const;
  FunctionBuilder Method(const std::string & name, NativeFunctionSignature func = nullptr) const;
  FunctionBuilder Operation(OperatorName op, NativeFunctionSignature func = nullptr) const;
  FunctionBuilder Conversion(const Type & dest, NativeFunctionSignature func = nullptr) const;

  ClassBuilder NestedClass(const std::string & name) const;

  const std::vector<Function> & memberFunctions() const;
  inline const std::vector<Function> & methods() const { return memberFunctions(); }

  bool isAbstract() const;
  const std::vector<Function> & vtable() const;

  typedef script::StaticDataMember StaticDataMember;

  void addStaticDataMember(const std::string & name, const Value & value, AccessSpecifier aspec = AccessSpecifier::Public);
  const std::map<std::string, StaticDataMember> & staticDataMembers() const;

  void addFriend(const Function & f);
  void addFriend(const Class & c);
  const std::vector<Function> & friends(const Function &) const;
  const std::vector<Class> & friends(const Class &) const;

  Class memberOf() const;
  Namespace enclosingNamespace() const;

  bool isTemplateInstance() const;
  ClassTemplate instanceOf() const;
  const std::vector<TemplateArgument> & arguments() const;

  Engine* engine() const;
  inline const std::shared_ptr<ClassImpl> & impl() const { return d; }

  Class & operator=(const Class & other) = default;
  bool operator==(const Class & other) const;
  bool operator!=(const Class & other) const;
  bool operator<(const Class & other) const;

private:
  std::shared_ptr<ClassImpl> d;
};

} // namespace script

#endif // LIBSCRIPT_CLASS_H
