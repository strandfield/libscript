// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_ERRORS_H
#define LIBSCRIPT_COMPILER_ERRORS_H

#include <string>

#include "script/accessspecifier.h"
#include "script/diagnosticmessage.h"
#include "script/exception.h"
#include "script/operators.h"
#include "script/types.h"

namespace script
{

namespace compiler
{

class CompilerException : public Exception
{
public:
  diagnostic::pos_t pos;

public:
  CompilerException() : pos{-1, -1} { }
  CompilerException(const CompilerException &) { }
  virtual ~CompilerException() = default;

  CompilerException(const diagnostic::pos_t & p) : pos(p) {}
};


#define CE(Name) public: \
  ~Name() = default; \
  ErrorCode code() const override { return ErrorCode::C_##Name; }


#define GENERIC_COMPILER_EXCEPTION(Name) class Name : public CompilerException \
{ \
public: \
  Name(diagnostic::pos_t p = diagnostic::pos_t{-1,-1}) : CompilerException(p) { } \
  ErrorCode code() const override { return ErrorCode::C_##Name; } \
  ~Name() = default; \
}


GENERIC_COMPILER_EXCEPTION(IllegalUseOfThis);
GENERIC_COMPILER_EXCEPTION(ObjectHasNoDestructor);
GENERIC_COMPILER_EXCEPTION(InvalidUseOfDelegatedConstructor);

class NotDataMember : public CompilerException
{
  CE(NotDataMember)
  std::string name;

  NotDataMember(diagnostic::pos_t p, const std::string & n) : CompilerException(p), name(n) { };
};

class InheritedDataMember : public CompilerException
{
  CE(InheritedDataMember)
  std::string name;

  InheritedDataMember(diagnostic::pos_t p, const std::string & n) : CompilerException(p), name(n) { };
};

class DataMemberAlreadyHasInitializer : public CompilerException
{ 
  CE(DataMemberAlreadyHasInitializer)
  std::string name;

  DataMemberAlreadyHasInitializer(diagnostic::pos_t p, const std::string & n) : CompilerException(p), name(n) { };
};

GENERIC_COMPILER_EXCEPTION(NoDelegatingConstructorFound);
GENERIC_COMPILER_EXCEPTION(CouldNotFindValidBaseConstructor);
GENERIC_COMPILER_EXCEPTION(InitializerListAsFirstArrayElement);
GENERIC_COMPILER_EXCEPTION(ReturnStatementWithoutValue);
GENERIC_COMPILER_EXCEPTION(ReturnStatementWithValue);
GENERIC_COMPILER_EXCEPTION(ReferencesMustBeInitialized);
GENERIC_COMPILER_EXCEPTION(EnumerationsCannotBeDefaultConstructed);
GENERIC_COMPILER_EXCEPTION(EnumerationsMustBeInitialized);
GENERIC_COMPILER_EXCEPTION(FunctionVariablesMustBeInitialized);

class VariableCannotBeDefaultConstructed : public CompilerException
{
  CE(VariableCannotBeDefaultConstructed)
  script::Type type;

  VariableCannotBeDefaultConstructed(diagnostic::pos_t p, const script::Type & t) : CompilerException(p), type(t) { }
};

class ClassHasDeletedDefaultCtor : public CompilerException
{
  CE(ClassHasDeletedDefaultCtor)
  script::Type type;

  ClassHasDeletedDefaultCtor(diagnostic::pos_t p, const script::Type & t) 
    : CompilerException(p), type(t) { }
};

class VariableCannotBeDestroyed : public CompilerException
{
  CE(VariableCannotBeDestroyed)
  script::Type type;

  VariableCannotBeDestroyed(const script::Type & t) : type(t) { }
};

GENERIC_COMPILER_EXCEPTION(CouldNotResolveOperatorName);

class InvalidParamCountInOperatorOverload : public CompilerException
{
  CE(InvalidParamCountInOperatorOverload)
  int expected;
  int actual;

  InvalidParamCountInOperatorOverload(diagnostic::pos_t p, int expect, int got) 
    : CompilerException(p)
    , expected(expect), actual(got) { }
};

class OpOverloadMustBeDeclaredAsMember : public CompilerException
{
  CE(OpOverloadMustBeDeclaredAsMember)
  script::OperatorName name;

  OpOverloadMustBeDeclaredAsMember(diagnostic::pos_t p, script::OperatorName n)
    : CompilerException(p)
    , name(n) { }
};

class InvalidTypeName : public CompilerException
{
  CE(InvalidTypeName)
  std::string name;

  InvalidTypeName(diagnostic::pos_t p, const std::string & n) : CompilerException(p), name(n) { }
};

GENERIC_COMPILER_EXCEPTION(DeclarationProcessingError);
GENERIC_COMPILER_EXCEPTION(DataMemberCannotBeAuto);
GENERIC_COMPILER_EXCEPTION(MissingStaticInitialization);
GENERIC_COMPILER_EXCEPTION(InvalidStaticInitialization);
GENERIC_COMPILER_EXCEPTION(FailedToInitializeStaticVariable);
GENERIC_COMPILER_EXCEPTION(InvalidBaseClass);
GENERIC_COMPILER_EXCEPTION(InvalidUseOfDefaultArgument);
GENERIC_COMPILER_EXCEPTION(ArrayElementNotConvertible);
GENERIC_COMPILER_EXCEPTION(ArraySubscriptOnNonObject);
GENERIC_COMPILER_EXCEPTION(CouldNotFindValidSubscriptOperator);
GENERIC_COMPILER_EXCEPTION(CannotCaptureThis);
GENERIC_COMPILER_EXCEPTION(UnknownCaptureName);
GENERIC_COMPILER_EXCEPTION(CannotCaptureNonCopyable);
GENERIC_COMPILER_EXCEPTION(SomeLocalsCannotBeCaptured);
GENERIC_COMPILER_EXCEPTION(CannotCaptureByValueAndByRef);
GENERIC_COMPILER_EXCEPTION(LambdaMustBeCaptureless);
GENERIC_COMPILER_EXCEPTION(CouldNotFindValidConstructor);
GENERIC_COMPILER_EXCEPTION(CouldNotFindValidMemberFunction);
GENERIC_COMPILER_EXCEPTION(CouldNotFindValidOperator);
GENERIC_COMPILER_EXCEPTION(CouldNotFindValidOverload);
GENERIC_COMPILER_EXCEPTION(CouldNotFindValidCallOperator);
GENERIC_COMPILER_EXCEPTION(AmbiguousFunctionName);
GENERIC_COMPILER_EXCEPTION(TemplateNamesAreNotExpressions);
GENERIC_COMPILER_EXCEPTION(TypeNameInExpression);
GENERIC_COMPILER_EXCEPTION(NamespaceNameInExpression);
GENERIC_COMPILER_EXCEPTION(TooManyArgumentInVariableInitialization);
GENERIC_COMPILER_EXCEPTION(TooManyArgumentInInitialization);
GENERIC_COMPILER_EXCEPTION(TooManyArgumentInReferenceInitialization);
GENERIC_COMPILER_EXCEPTION(TooManyArgumentsInMemberInitialization);

class CouldNotConvert : public CompilerException
{
  CE(CouldNotConvert)
  script::Type source;
  script::Type destination;

  CouldNotConvert(diagnostic::pos_t p, const script::Type & src, const script::Type & dest) 
    : CompilerException(p)
    , source(src), destination(dest) { }
};

class CouldNotFindCommonType : public CompilerException
{
  CE(CouldNotFindCommonType)

  script::Type first;
  script::Type second;

  CouldNotFindCommonType(diagnostic::pos_t p, const script::Type & t1, const script::Type & t2) 
    : CompilerException(p), first(t1), second(t2) { }
};

GENERIC_COMPILER_EXCEPTION(CannotAccessMemberOfNonObject);
GENERIC_COMPILER_EXCEPTION(NoSuchMember);
GENERIC_COMPILER_EXCEPTION(InvalidTemplateArgument);
GENERIC_COMPILER_EXCEPTION(InvalidLiteralTemplateArgument);
GENERIC_COMPILER_EXCEPTION(NonConstExprTemplateArgument);
GENERIC_COMPILER_EXCEPTION(InvalidTemplateArgumentType);
GENERIC_COMPILER_EXCEPTION(MissingNonDefaultedTemplateParameter);
GENERIC_COMPILER_EXCEPTION(CouldNotFindPrimaryClassTemplate);
GENERIC_COMPILER_EXCEPTION(CouldNotFindPrimaryFunctionTemplate);
GENERIC_COMPILER_EXCEPTION(InvalidUseOfConstKeyword);
GENERIC_COMPILER_EXCEPTION(InvalidUseOfExplicitKeyword);
GENERIC_COMPILER_EXCEPTION(InvalidUseOfStaticKeyword);
GENERIC_COMPILER_EXCEPTION(InvalidUseOfVirtualKeyword);
GENERIC_COMPILER_EXCEPTION(AutoMustBeUsedWithAssignment);
GENERIC_COMPILER_EXCEPTION(CannotDeduceLambdaReturnType);
GENERIC_COMPILER_EXCEPTION(CallToDeletedFunction);
GENERIC_COMPILER_EXCEPTION(FunctionCannotBeDefaulted);
GENERIC_COMPILER_EXCEPTION(ParentHasNoDefaultConstructor);
GENERIC_COMPILER_EXCEPTION(ParentHasDeletedDefaultConstructor);
GENERIC_COMPILER_EXCEPTION(ParentHasNoCopyConstructor);
GENERIC_COMPILER_EXCEPTION(ParentHasDeletedCopyConstructor);
GENERIC_COMPILER_EXCEPTION(DataMemberIsNotCopyable);
GENERIC_COMPILER_EXCEPTION(ParentHasNoMoveConstructor);
GENERIC_COMPILER_EXCEPTION(ParentHasDeletedMoveConstructor);
GENERIC_COMPILER_EXCEPTION(DataMemberIsNotMovable);
GENERIC_COMPILER_EXCEPTION(ParentHasNoAssignmentOperator);
GENERIC_COMPILER_EXCEPTION(ParentHasDeletedAssignmentOperator);
GENERIC_COMPILER_EXCEPTION(DataMemberHasNoAssignmentOperator);
GENERIC_COMPILER_EXCEPTION(DataMemberHasDeletedAssignmentOperator);
GENERIC_COMPILER_EXCEPTION(DataMemberIsReferenceAndCannotBeAssigned);
GENERIC_COMPILER_EXCEPTION(InvalidArgumentCountInDataMemberRefInit);
GENERIC_COMPILER_EXCEPTION(CannotInitializeNonConstRefDataMemberWithConst);

class BadDataMemberRefInit : public CompilerException
{
  CE(BadDataMemberRefInit)
  std::string name;
  BadDataMemberRefInit(const std::string & n) : name(n) { }
};

class EnumMemberCannotBeDefaultConstructed : public CompilerException
{
  CE(EnumMemberCannotBeDefaultConstructed)
  script::Type type;
  std::string name;
  EnumMemberCannotBeDefaultConstructed(const script::Type & t, const std::string & n) : type(t), name(n) { }
};

class DataMemberHasNoDefaultConstructor : public CompilerException
{
  CE(DataMemberHasNoDefaultConstructor)
  script::Type type;
  std::string name;

  DataMemberHasNoDefaultConstructor(const script::Type & t, const std::string & n) : type(t), name(n) { }
};

class DataMemberHasDeletedDefaultConstructor : public CompilerException
{
  CE(DataMemberHasDeletedDefaultConstructor)

  script::Type type;
  std::string name;

  DataMemberHasDeletedDefaultConstructor(const script::Type & t, const std::string & n) : type(t), name(n) { }
};

GENERIC_COMPILER_EXCEPTION(InvalidCharacterLiteral);
GENERIC_COMPILER_EXCEPTION(CouldNotFindValidLiteralOperator);

class UnknownTypeInBraceInitialization : public CompilerException
{
  CE(UnknownTypeInBraceInitialization)
  std::string name;
  UnknownTypeInBraceInitialization(diagnostic::pos_t p, const std::string & n) 
    : CompilerException(p)
    , name(n) { }
};

class NarrowingConversionInBraceInitialization : public CompilerException
{
  CE(NarrowingConversionInBraceInitialization)

  script::Type source;
  script::Type destination;

  NarrowingConversionInBraceInitialization(const script::Type & src, const script::Type & dest)
    : source(src), destination(dest) { }
  NarrowingConversionInBraceInitialization(diagnostic::pos_t p, const script::Type & src, const script::Type & dest) 
    : CompilerException(p), source(src), destination(dest) { }
};

GENERIC_COMPILER_EXCEPTION(NamespaceDeclarationCannotAppearAtThisLevel);
GENERIC_COMPILER_EXCEPTION(ExpectedDeclaration);
GENERIC_COMPILER_EXCEPTION(GlobalVariablesCannotBeAuto);
GENERIC_COMPILER_EXCEPTION(GlobalVariablesMustBeInitialized);
GENERIC_COMPILER_EXCEPTION(GlobalVariablesMustBeAssigned);

class InaccessibleMember : public CompilerException
{
  CE(InaccessibleMember)
  std::string name;
  script::AccessSpecifier access;

  InaccessibleMember(diagnostic::pos_t p, const std::string & n, script::AccessSpecifier a) 
    : CompilerException(p)
    , name(n), access(a) { }
};

GENERIC_COMPILER_EXCEPTION(FriendMustBeAClass);

class UnknownModuleName : public CompilerException
{
  CE(UnknownModuleName)
  std::string name;

  UnknownModuleName(diagnostic::pos_t p, const std::string & n) : CompilerException(p), name(n) { }
};

class UnknownSubModuleName : public CompilerException
{
  CE(UnknownSubModuleName)
  std::string name;
  std::string moduleName;

  UnknownSubModuleName(diagnostic::pos_t p, const std::string & n, const std::string & modn) 
    : CompilerException(p), name(n), moduleName(modn) { }
};

class ModuleImportationError : public CompilerException
{
  CE(ModuleImportationError)
  std::string name;
  std::string message;

  ModuleImportationError(const std::string & n, const std::string & mssg) : name(n), message(mssg) { }
};

class InvalidNameInUsingDirective : public CompilerException
{
  CE(InvalidNameInUsingDirective)
  std::string name;

  InvalidNameInUsingDirective(diagnostic::pos_t p, const std::string & n) : CompilerException(p), name(n) { }
};

GENERIC_COMPILER_EXCEPTION(NoSuchCallee);
GENERIC_COMPILER_EXCEPTION(LiteralOperatorNotInNamespace);

#undef CE
#undef GENERIC_COMPILER_EXCEPTION

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_ERRORS_H
