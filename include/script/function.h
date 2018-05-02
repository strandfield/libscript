// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_H
#define LIBSCRIPT_FUNCTION_H

#include "prototype.h"
#include "value.h"

namespace script
{

enum class AccessSpecifier;
class Cast;
class Class;
class Engine;
namespace interpreter { class FunctionCall; }
typedef interpreter::FunctionCall FunctionCall;
namespace program { class Statement; }
class FunctionImpl;
class FunctionTemplate;
class LiteralOperator;
class Operator;
class Script;
class TemplateArgument;
class UserData;

typedef Value (*NativeFunctionSignature) (FunctionCall *);

class LIBSCRIPT_API Function
{
public:
  Function() = default;
  Function(const Function & other) = default;
  ~Function() = default;

  Function(const std::shared_ptr<FunctionImpl> & impl);

  enum Kind {
    StandardFunction = 0,
    Constructor = 1,
    Destructor = 2,
    OperatorFunction = 3,
    CastFunction = 4,
    Root = 5
  };

  enum Flag {
    NoFlags = 0,
    // Static = 1, // Static functions are just free functions defined in a class, not need for a flag
    Explicit = 2,
    Virtual = 4,
    Pure = 8,
    ConstExpr = 16,
    Default = 32,
    Delete = 64,
    Protected = 128,
    Private = 256,
  };

  enum ImplementationMethod {
    NativeFunction = 0,
    InterpretedFunction = 1
  };

  bool isNull() const;

  std::string name() const;
  const Prototype & prototype() const;
  std::vector<Prototype> prototypes() const;
  bool accepts(int argc) const;
  const Type & parameter(int index) const;
  const Type & returnType() const;

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
  Class memberOf() const;
  AccessSpecifier accessibility() const;

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
  FunctionImpl * implementation() const;
  std::weak_ptr<FunctionImpl> weakref() const;

  Function & operator=(const Function & other);
  bool operator==(const Function & other) const;
  bool operator!=(const Function & other) const;

protected:
  std::shared_ptr<FunctionImpl> d;
};


} // namespace script

#endif // LIBSCRIPT_FUNCTION_H
