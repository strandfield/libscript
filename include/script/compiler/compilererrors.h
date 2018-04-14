// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_ERRORS_H
#define LIBSCRIPT_COMPILER_ERRORS_H

#include <string>

#include "script/diagnosticmessage.h"
#include "script/exception.h"

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
  CompilerException(const CompilerException &) = default;
  virtual ~CompilerException() = default;

  CompilerException(const diagnostic::pos_t & p) : pos(p) {}

  virtual std::string what() const = 0;
};

class CCompilerExceptionBase : public CompilerException
{
public:
  CCompilerExceptionBase() = default;
  CCompilerExceptionBase(const diagnostic::pos_t & p) : CompilerException(p) { }
  virtual std::string get_format_message() const = 0;
};

template<typename...Args>
class CCompilerException : public CCompilerExceptionBase
{
public:
  typedef std::tuple<Args...>  container_type;
  container_type items;

  CCompilerException(const Args &... args)
    : items{ args... }
  {

  }

  CCompilerException(diagnostic::pos_t p, const Args &... args)
    : CCompilerExceptionBase(p)
    , items{ args... }
  {

  }
  template<std::size_t...I>
  std::string what_impl(std::index_sequence<I...>) const
  {
    return diagnostic::format(this->get_format_message(), std::get<I>(items)...);
  }

  std::string what() const override
  {
    return what_impl(std::make_index_sequence<sizeof...(Args)>{});
  }
};

#define DECLARE_COMPILER_ERROR(name, mssg, ...) class name : public CCompilerException<__VA_ARGS__> \
{ \
public: \
  using CCompilerException<__VA_ARGS__>::CCompilerException; \
  std::string get_format_message() const override { return mssg; } \
};


DECLARE_COMPILER_ERROR(IllegalUseOfThis, "Illegal use of this");
DECLARE_COMPILER_ERROR(ObjectHasNoDestructor, "Object has no destructor");
DECLARE_COMPILER_ERROR(InvalidUseOfDelegatedConstructor, "No other member initializer may be present when using delegating constructors");
DECLARE_COMPILER_ERROR(NotDataMember, "%1 does not name a data member", std::string);
DECLARE_COMPILER_ERROR(InheritedDataMember, "cannot initialize inherited data member %1", std::string);
DECLARE_COMPILER_ERROR(DataMemberAlreadyHasInitializer, "data member %1 already has an intializer", std::string);
DECLARE_COMPILER_ERROR(NoDelegatingConstructorFound, "Could not find a delegate constructor");
DECLARE_COMPILER_ERROR(CouldNotFindValidBaseConstructor, "Could not find valid base constructor");
DECLARE_COMPILER_ERROR(InitializerListAsFirstArrayElement, "An initializer list cannot be used as the first element of an array");

DECLARE_COMPILER_ERROR(ReturnStatementWithoutValue, "Cannot have return-statement without a value in function returning non-void");
DECLARE_COMPILER_ERROR(ReturnStatementWithValue, "A function returning void cannot return a value");

DECLARE_COMPILER_ERROR(ReferencesMustBeInitialized, "References must be initialized");
DECLARE_COMPILER_ERROR(EnumerationsCannotBeDefaultConstructed, "Enumerations cannot be default constructed");
DECLARE_COMPILER_ERROR(EnumerationsMustBeInitialized, "Variables of enumeration type must be initialized");
DECLARE_COMPILER_ERROR(FunctionVariablesMustBeInitialized, "Variables of function-type must be initialized");
DECLARE_COMPILER_ERROR(VariableCannotBeDefaultConstructed, "Class '%1' does not provide a default constructor", std::string);
DECLARE_COMPILER_ERROR(ClassHasDeletedDefaultCtor, "Class '%1' has a deleted default constructor", std::string);
DECLARE_COMPILER_ERROR(VariableCannotBeDestroyed, "Class '%1' does not provide a destructor", std::string);

DECLARE_COMPILER_ERROR(CouldNotResolveOperatorName, "Could not resolve operator name based on parameter count and operator symbol.");
DECLARE_COMPILER_ERROR(InvalidParamCountInOperatorOverload, "Invalid parameter count found in operator overload, expected %1 got %2", std::string, std::string);
DECLARE_COMPILER_ERROR(OpOverloadMustBeDeclaredAsMember, "This operator can only be overloaded as a member");

DECLARE_COMPILER_ERROR(InvalidTypeName, "%1 does not name a type", std::string);

DECLARE_COMPILER_ERROR(DeclarationProcessingError, "Some declarations could not be processed.");


DECLARE_COMPILER_ERROR(DataMemberCannotBeAuto, "Data members cannot be declared 'auto'.");
DECLARE_COMPILER_ERROR(MissingStaticInitialization, "A static variable must be initialized.");
DECLARE_COMPILER_ERROR(InvalidStaticInitialization, "Static variables can only be initialized through assignment.");
DECLARE_COMPILER_ERROR(FailedToInitializeStaticVariable, "Failed to initialized static variable.");

DECLARE_COMPILER_ERROR(InvalidBaseClass, "Invalid base class.");

DECLARE_COMPILER_ERROR(InvalidUseOfDefaultArgument, "Cannot have a parameter without a default value after one was provided.");

DECLARE_COMPILER_ERROR(ArrayElementNotConvertible, "Could not convert element to array's element type.");
DECLARE_COMPILER_ERROR(ArraySubscriptOnNonObject, "Cannot perform array subscript on non object type.");
DECLARE_COMPILER_ERROR(CouldNotFindValidSubscriptOperator, "Could not find valid subscript operator.");


DECLARE_COMPILER_ERROR(CannotCaptureThis, "'this' cannot be captured outside of a member function.");
DECLARE_COMPILER_ERROR(UnknownCaptureName, "Could not capture any local variable with given name.");
DECLARE_COMPILER_ERROR(CannotCaptureNonCopyable, "Cannot capture by value a non copyable type.");
DECLARE_COMPILER_ERROR(SomeLocalsCannotBeCaptured, "Some local variables cannot be captured by value.");
DECLARE_COMPILER_ERROR(CannotCaptureByValueAndByRef, "Cannot capture both everything by reference and by value.");
DECLARE_COMPILER_ERROR(LambdaMustBeCaptureless, "A lambda must be captureless within this context.");

DECLARE_COMPILER_ERROR(CouldNotFindValidConstructor, "Could not find valid constructor.");
DECLARE_COMPILER_ERROR(CouldNotFindValidMemberFunction, "Could not find valid member function for call.");
DECLARE_COMPILER_ERROR(CouldNotFindValidOperator, "Could not find valid operator overload.");
DECLARE_COMPILER_ERROR(CouldNotFindValidOverload, "Overload resolution failed.");
DECLARE_COMPILER_ERROR(CouldNotFindValidCallOperator, "Could not find valid operator() overload for call.");

DECLARE_COMPILER_ERROR(AmbiguousFunctionName, "Name does not refer to a single function");
DECLARE_COMPILER_ERROR(TemplateNamesAreNotExpressions, "Name refers to a template and cannot be used inside an expression");
DECLARE_COMPILER_ERROR(TypeNameInExpression, "Name refers to a type and cannot be used inside an expression");

DECLARE_COMPILER_ERROR(NamespaceNameInExpression, "Name refers to a namespace and cannot be used inside an expression");

DECLARE_COMPILER_ERROR(TooManyArgumentInVariableInitialization, "Too many arguments provided in variable initialization.");
DECLARE_COMPILER_ERROR(TooManyArgumentInInitialization, "Too many arguments provided in initialization.");
DECLARE_COMPILER_ERROR(TooManyArgumentInReferenceInitialization, "More than one argument provided in reference initialization.");
DECLARE_COMPILER_ERROR(TooManyArgumentsInMemberInitialization, "Too many arguments in member initialization.");

DECLARE_COMPILER_ERROR(CouldNotConvert, "Could not convert from %1 to %2", std::string, std::string);

DECLARE_COMPILER_ERROR(CannotAccessMemberOfNonObject, "Cannot access member of non object type.");
DECLARE_COMPILER_ERROR(NoSuchMember, "Object has no such member.");


DECLARE_COMPILER_ERROR(InvalidTemplateArgument, "Invalid template argument.");
DECLARE_COMPILER_ERROR(InvalidLiteralTemplateArgument, "Only integer and boolean literals can be used as template arguments.");
DECLARE_COMPILER_ERROR(NonConstExprTemplateArgument, "Template arguments must be constant expressions.");
DECLARE_COMPILER_ERROR(InvalidTemplateArgumentType, "This constant epression does not evaluate to an int or a bool.");

DECLARE_COMPILER_ERROR(InvalidUseOfVirtualKeyword, "Invalid use of virtual keyword.");

DECLARE_COMPILER_ERROR(AutoMustBeUsedWithAssignment, "'auto' can only be used with assignment initialization.");

DECLARE_COMPILER_ERROR(CannotDeduceLambdaReturnType, "Cannot deduce lambda return type");

DECLARE_COMPILER_ERROR(CallToDeletedFunction, "Call to deleted function.");

DECLARE_COMPILER_ERROR(FunctionCannotBeDefaulted, "Function cannot be defaulted.");

DECLARE_COMPILER_ERROR(ParentHasNoCopyConstructor, "Cannot generate defaulted copy constructor because parent has no copy constructor.");
DECLARE_COMPILER_ERROR(ParentHasDeletedCopyConstructor, "Cannot generate defaulted copy constructor because parent copy constructor is deleted.");
DECLARE_COMPILER_ERROR(DataMemberIsNotCopyable, "Cannot generate defaulted copy constructor because at least one data member is not copyable.");
DECLARE_COMPILER_ERROR(ParentHasNoMoveConstructor, "Cannot generate defaulted move constructor because parent has no move constructor.");
DECLARE_COMPILER_ERROR(ParentHasDeletedMoveConstructor, "Cannot generate defaulted move constructor because parent move constructor is deleted.");
DECLARE_COMPILER_ERROR(DataMemberIsNotMovable, "Cannot generate defaulted move constructor because at least one data member is not movable.");

DECLARE_COMPILER_ERROR(ParentHasNoAssignmentOperator, "Cannot generate defaulted assignment operator because parent has no assignment operator.");
DECLARE_COMPILER_ERROR(ParentHasDeletedAssignmentOperator, "Cannot generate defaulted assignment operator because parent has a deleted assignment operator.");
DECLARE_COMPILER_ERROR(DataMemberHasNoAssignmentOperator, "Cannot generate defaulted assignment operator because at least one data member has no assignment operator.");
DECLARE_COMPILER_ERROR(DataMemberHasDeletedAssignmentOperator, "Cannot generate defaulted assignment operator because at least one data member has a deleted assignment operator.");

DECLARE_COMPILER_ERROR(DataMemberIsReferenceAndCannotBeAssigned, "Cannot generate defaulted assignment operator because at least one data member is a reference.");

DECLARE_COMPILER_ERROR(InvalidArgumentCountInDataMemberRefInit, "Only one value must be provided to initialize a data member of reference type.");
DECLARE_COMPILER_ERROR(CannotInitializeNonConstRefDataMemberWithConst, "Cannot initialize a data member of non-const reference type with a const value.");
DECLARE_COMPILER_ERROR(BadDataMemberRefInit, "Bad reference initialization of data member %1.", std::string);
DECLARE_COMPILER_ERROR(EnumMemberCannotBeDefaultConstructed, "Data member %1 of enumeration type %2 cannot be default constructed.", std::string, std::string);
DECLARE_COMPILER_ERROR(DataMemberHasNoDefaultConstructor, "Data member %1 of type %2 has no default constructor.", std::string, std::string);
DECLARE_COMPILER_ERROR(DataMemberHasDeletedDefaultConstructor, "Data member %1 of type %2 has a deleted default constructor.", std::string, std::string);

DECLARE_COMPILER_ERROR(InvalidCharacterLiteral, "A character literal must contain only one character.");

DECLARE_COMPILER_ERROR(CouldNotFindValidLiteralOperator, "Could not find valid literal operator.");

DECLARE_COMPILER_ERROR(UnknownTypeInBraceInitialization, "Unknown type %1 in brace initialization", std::string);
DECLARE_COMPILER_ERROR(NarrowingConversionInBraceInitialization, "Narrowing conversion from %1 to %2 in brace initialization", std::string, std::string);

DECLARE_COMPILER_ERROR(NamespaceDeclarationCannotAppearAtThisLevel, "Namespace declarations cannot appear at this level");
DECLARE_COMPILER_ERROR(ExpectedDeclaration, "Expected a declaration.");
DECLARE_COMPILER_ERROR(GlobalVariablesCannotBeAuto, "Global variables cannot be declared with auto.");
DECLARE_COMPILER_ERROR(GlobalVariablesMustBeInitialized, "Global variables must have an initializer.");
DECLARE_COMPILER_ERROR(GlobalVariablesMustBeAssigned, "Global variables must be initialized through assignment.");

DECLARE_COMPILER_ERROR(InaccessibleMember, "%1 is %2 within this context", std::string, std::string);

DECLARE_COMPILER_ERROR(FriendMustBeAClass, "Friend must be a class");

DECLARE_COMPILER_ERROR(NotImplementedError, "Not implemented : %1", std::string);

#undef DECLARE_COMPILER_ERROR

} // namespace compiler

} // namespace script


#endif // LIBSCRIPT_COMPILER_ERRORS_H
