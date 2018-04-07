// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_CLASS_H
#define LIBSCRIPT_CLASS_H

#include "libscriptdefs.h"
#include "function.h"
#include "operator.h"

#include <map>

namespace script
{

class ClassImpl;
class Enum;
class Script;
class UserData;
class ClassBuilder;
class FunctionBuilder;
class Template;
class Typedef;

class LIBSCRIPT_API Class
{
public:
  Class() = default;
  Class(const Class & other) = default;
  ~Class() = default;
  
  Class(const std::shared_ptr<ClassImpl> & impl);

  bool isNull() const;
  int id() const;
  
  const std::string & name() const;

  Class parent() const;
  bool inherits(const Class & type) const;
  int inheritanceLevel(const Class & type) const;
  bool isFinal() const;


  /// TODO : add access specifier
  struct DataMember
  {
    Type type;
    std::string name;
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

  Enum newEnum(const std::string & src);
  const std::vector<Enum> & enums() const;

  const std::vector<Template> & templates() const;
  const std::vector<Typedef> & typedefs() const;

  const std::vector<Operator> & operators() const;
  const std::vector<Cast> & casts() const;

  Function newConstructor(const Prototype & proto, NativeFunctionSignature func, uint8 flags = Function::NoFlags);
  Function newConstructor(const FunctionBuilder & builder);
  const std::vector<Function> & constructors() const;
  Function defaultConstructor() const;
  bool isDefaultConstructible() const;
  Function copyConstructor() const;
  Function moveConstructor() const;
  bool isCopyConstructible() const;
  bool isMoveConstructible() const;

  Function newDestructor(NativeFunctionSignature func);
  Function destructor() const;
  
  Function newMethod(const std::string & name, const Prototype & proto, NativeFunctionSignature func, uint8 flags = Function::NoFlags);
  Function newMethod(const FunctionBuilder & builder);

  Operator newOperator(const FunctionBuilder & builder);
  Cast newCast(const FunctionBuilder & builder);

  FunctionBuilder Method(const std::string & name, NativeFunctionSignature func = nullptr) const;
  FunctionBuilder Operation(Operator::BuiltInOperator op, NativeFunctionSignature func = nullptr) const;
  FunctionBuilder Conversion(const Type & dest, NativeFunctionSignature func = nullptr) const;

  const std::vector<Function> & memberFunctions() const;
  inline const std::vector<Function> & methods() const { return memberFunctions(); }

  bool isAbstract() const;
  const std::vector<Function> & vtable() const;

  /// TODO : add access specifier
  struct StaticDataMember
  {
    std::string name;
    Value value;
  };

  void addStaticDataMember(const std::string & name, const Value & value);
  const std::map<std::string, StaticDataMember> & staticDataMembers() const;

  Engine * engine() const;
  ClassImpl * implementation() const;
  std::weak_ptr<ClassImpl> weakref() const;

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

public:
  ClassBuilder(const std::string & n, const Class & p = Class{});

  static ClassBuilder New(const std::string & n);

  ClassBuilder & setParent(const Class & p);
  ClassBuilder & setFinal(bool f = true);
  ClassBuilder & addMember(const Class::DataMember & dm);
  ClassBuilder & setData(const std::shared_ptr<UserData> & data);
};

} // namespace script




#endif // LIBSCRIPT_CLASS_H
