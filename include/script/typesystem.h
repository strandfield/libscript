// Copyright (C) 2019 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TYPESYSTEM_H
#define LIBSCRIPT_TYPESYSTEM_H

#include <vector>

#include "script/types.h"

namespace script
{

class Array;
class Class;
class ClosureType;
class Conversion;
class Engine;
class Enum;
class FunctionType;
class Namespace;
class Prototype;
class Scope;
class Script;
class TypeSystemListener;

class TypeSystemImpl;

class LIBSCRIPT_API TypeSystem
{
public:
  TypeSystem() = delete;
  TypeSystem(const TypeSystem& other) = delete;
  ~TypeSystem();

  explicit TypeSystem(std::unique_ptr<TypeSystemImpl>&& impl);
  
  Engine* engine() const;

  bool exists(const Type& t) const;
  Class getClass(Type id) const;
  Enum getEnum(Type id) const;
  ClosureType getLambda(Type id) const;
  FunctionType getFunctionType(Type id) const;
  FunctionType getFunctionType(const Prototype& proto);

  bool isDefaultConstructible(const Type& t) const;
  bool isCopyConstructible(const Type& t) const;
  bool isCopiable(const Type & t) const;
  bool isMoveConstructible(const Type& t) const;

  Conversion conversion(const Type & src, const Type & dest) const;
  bool canConvert(const Type & srcType, const Type & destType) const;

  Type typeId(const std::string & typeName) const;
  Type typeId(const std::string& typeName, const Scope& scope) const;
  std::string typeName(Type t) const;

  bool isInitializerList(const Type & t) const;

  size_t reserve(Type::TypeFlag flag, size_t count);

  void addListener(TypeSystemListener* listener);
  void removeListener(TypeSystemListener* listener);

  bool hasActiveTransaction() const;

  TypeSystemImpl* impl() const;

  TypeSystem& operator=(const TypeSystem& ) = delete;

protected:
  std::unique_ptr<TypeSystemImpl> d;
};

inline bool TypeSystem::isCopiable(const Type& t) const { return isCopyConstructible(t); }

} // namespace script

#endif // LIBSCRIPT_TYPESYSTEM_H
