// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_CLASS_H
#define LIBSCRIPT_CLASS_H

#include "script/accessspecifier.h"
#include "script/function.h"
#include "script/operators.h"
#include "script/value.h"

#include <map>

namespace script
{

class ClassImpl;
class ClosureType;
class Enum;
class Script;
class UserData;
class ClassBuilder;
class ClassTemplate;
class FunctionBuilder;
class Namespace;
class Template;
class TemplateArgument;
class Typedef;

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

  class DataMember
  {
  public:
    Type type;
    std::string name;

    DataMember() = default;
    DataMember(const DataMember &) = default;
    DataMember(const Type & t, const std::string & name, AccessSpecifier aspec = AccessSpecifier::Public);
    ~DataMember() = default;

    DataMember & operator=(const DataMember &) = default;

    inline bool isNull() const { return name.empty(); }
    AccessSpecifier accessibility() const;
  };

  const std::vector<DataMember> & dataMembers() const;
  /// TODO : add cumulatedDataMembers()
  // std::vector<DataMember> cumulatedDataMembers() const;
  int cumulatedDataMemberCount() const;
  int attributesOffset() const;
  int attributeIndex(const std::string & attrName) const;


  Script script() const;

  const std::shared_ptr<UserData> & data() const;

  Value instantiate(const std::vector<Value> & args);

  Class newClass(const ClassBuilder & opts);
  const std::vector<Class> & classes() const;

  Enum newEnum(const std::string & name, int id = -1);
  const std::vector<Enum> & enums() const;

  const std::vector<Template> & templates() const;
  const std::vector<Typedef> & typedefs() const;

  const std::vector<Operator> & operators() const;
  const std::vector<Cast> & casts() const;

  [[deprecated("use Class::Constructor instead")]] Function newConstructor(const Prototype & proto, NativeFunctionSignature func, uint8 flags = Function::NoFlags);
  [[deprecated("use Class::Constructor instead")]] Function newConstructor(const FunctionBuilder & builder);
  const std::vector<Function> & constructors() const;
  Function defaultConstructor() const;
  bool isDefaultConstructible() const;
  Function copyConstructor() const;
  Function moveConstructor() const;
  bool isCopyConstructible() const;
  bool isMoveConstructible() const;

  Function newDestructor(NativeFunctionSignature func);
  Function destructor() const;
  
  [[deprecated("use Class::Method() instead")]] Function newMethod(const std::string & name, const Prototype & proto, NativeFunctionSignature func, uint8 flags = Function::NoFlags);
  [[deprecated("use Class::Method() instead")]] Function newMethod(const FunctionBuilder & builder);

  Operator newOperator(const FunctionBuilder & builder);
  [[deprecated("use Class::Conversion instead")]] Cast newCast(const FunctionBuilder & builder);

  FunctionBuilder Constructor(NativeFunctionSignature func = nullptr) const;
  FunctionBuilder Method(const std::string & name, NativeFunctionSignature func = nullptr) const;
  FunctionBuilder Operation(OperatorName op, NativeFunctionSignature func = nullptr) const;
  FunctionBuilder Conversion(const Type & dest, NativeFunctionSignature func = nullptr) const;

  const std::vector<Function> & memberFunctions() const;
  inline const std::vector<Function> & methods() const { return memberFunctions(); }

  bool isAbstract() const;
  const std::vector<Function> & vtable() const;

  class StaticDataMember
  {
  public:
    std::string name;
    Value value;

    StaticDataMember() = default;
    StaticDataMember(const StaticDataMember &) = default;
    StaticDataMember(const std::string &n, const Value & val, AccessSpecifier aspec = AccessSpecifier::Public);
    ~StaticDataMember() = default;

    StaticDataMember & operator=(const StaticDataMember &) = default;

    inline bool isNull() const { return this->value.isNull(); }
    AccessSpecifier accessibility() const;
  };

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

  Engine * engine() const;
  inline const std::shared_ptr<ClassImpl> & impl() const { return d; }

  Class & operator=(const Class & other) = default;
  bool operator==(const Class & other) const;
  bool operator!=(const Class & other) const;
  bool operator<(const Class & other) const;

private:
  std::shared_ptr<ClassImpl> d;
};


class LIBSCRIPT_API ClassBuilder
{
public:
  std::string name;
  Class parent;
  std::vector<Class::DataMember> dataMembers;
  bool isFinal;
  std::shared_ptr<UserData> userdata;
  int id;

public:
  ClassBuilder(const std::string & n, const Class & p = Class{});

  static ClassBuilder New(const std::string & n);

  ClassBuilder & setParent(const Class & p);
  ClassBuilder & setFinal(bool f = true);
  ClassBuilder & addMember(const Class::DataMember & dm);
  ClassBuilder & setData(const std::shared_ptr<UserData> & data);
  ClassBuilder & setId(int n);
};

} // namespace script




#endif // LIBSCRIPT_CLASS_H
