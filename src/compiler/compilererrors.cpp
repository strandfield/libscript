// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/compilererrors.h"

#include "script/diagnosticmessage.h"

namespace script
{

namespace compiler
{

CompilerException::CompilerException()
  : line(-1)
  , column(-1)
{

}

CompilerException::CompilerException(int l, int c)
  : line(l)
  , column(c)
{

}



const std::string IllegalUseOfThis::message = "Illegal use of this";
const std::string ObjectHasNoDestructor::message = "Object has no destructor";
const std::string InvalidUseOfDelegatedConstructor::message = "No other member initializer may be present when using delegating constructors";
const std::string NotDataMember::message = "... does not name a data member";
const std::string InheritedDataMember::message = "cannot initialize inherited data member";
const std::string DataMemberAlreadyHasInitializer::message = "data member already has an intializer";
const std::string NoDelegatingConstructorFound::message = "Could not find a delegate constructor";
const std::string CouldNotFindValidBaseConstructor::message = "Could not find valid base constructor";
const std::string InitializerListAsFirstArrayElement::message = "An initializer list cannot be used as the first element of an array";

const std::string ReturnStatementWithoutValue::message = "Cannot have return-statement without a value in function returning non-void";
const std::string ReturnStatementWithValue::message = "A function returning void cannot return a value";

const std::string ReferencesMustBeInitialized::message = "References must be initialized";
const std::string EnumerationsMustBeInitialized::message = "Variables of enumeration type must be initialized";
const std::string FunctionVariablesMustBeInitialized::message = "Variables of function-type must be initialized";
const std::string VariableCannotBeDefaultConstructed::message = "Class does not provide a default constructor";
const std::string VariableCannotBeDestroyed::message = "Class does not provide a destructor";

const std::string TooManyArgumentsInOperatorOverload::message = "Too many parameters provided for operator overload";
const std::string InvalidParamCountInOperatorOverload::message = "Invalid parameter count found in operator overload";
const std::string OpOverloadMustBeDeclaredAsMember::message = "This operator can only be overloaded as a member";

InvalidTypeName::InvalidTypeName(int l, int c, const std::string & n)
  : CompilerException(l, c)
  , name(n)
{
}

std::string InvalidTypeName::what() const
{
  return this->name + std::string{ " does not name a type" };
}

const std::string DeclarationProcessingError::message = "Some declarations could not be processed.";


const std::string DataMemberCannotBeAuto::message = "Data members cannot be declared 'auto'.";
const std::string MissingStaticInitialization::message = "A static variable must be initialized.";
const std::string InvalidStaticInitialization::message = "Static variables can only be initialized through assignment.";

const std::string InvalidBaseClass::message = "Invalid base class.";

const std::string InvalidUseOfDefaultArgument::message = "Cannot have a parameter without a default value after one was provided.";

const std::string ArrayElementNotConvertible::message = "Could not convert element to array's element type.";
const std::string ArraySubscriptOnNonObject::message = "Cannot perform array subscript on non object type.";
const std::string CouldNotFindValidSubscriptOperator::message = "Could not find valid subscript operator.";


const std::string CannotCaptureThis::message = "'this' cannot be captured outside of a member function.";
const std::string UnknownCaptureName::message = "Could not capture any local variable with given name.";
const std::string CannotCaptureNonCopyable::message = "Cannot capture by value a non copyable type.";
const std::string SomeLocalsCannotBeCaptured::message = "Some local variables cannot be captured by value.";
const std::string CannotCaptureByValueAndByRef::message = "Cannot capture both everything by reference and by value.";
const std::string LambdaMustBeCaptureless::message = "A lambda must be captureless within this context.";

const std::string CouldNotFindValidConstructor::message = "Could not find valid constructor.";
const std::string CouldNotFindValidMemberFunction::message = "Could not find valid member function for call.";
const std::string CouldNotFindValidOperator::message = "Could not find valid operator overload.";
const std::string CouldNotFindValidOverload::message = "Overload resolution failed.";
const std::string CouldNotFindValidCallOperator::message = "Could not find valid operator() overload for call.";

const std::string AmbiguousFunctionName::message = "Name does not refer to a single function";
const std::string TemplateNamesAreNotExpressions::message = "Name refers to a template and cannot be used inside an expression";
const std::string TypeNameInExpression::message = "Name refers to a type and cannot be used inside an expression";

const std::string NamespaceNameInExpression::message = "Name refers to a namespace and cannot be used inside an expression";

const std::string TooManyArgumentInVariableInitialization::message = "Too many arguments provided in variable initialization.";
const std::string TooManyArgumentInReferenceInitialization::message = "More than one argument provided in reference initialization.";
const std::string TooManyArgumentsInMemberInitialization::message = "Too many arguments in member initialization.";

CouldNotConvert::CouldNotConvert(int l, int c, const std::string & src, const std::string & dest)
  : CompilerException(l, c)
  , source_type(src)
  , dest_type(dest)
{

}

std::string CouldNotConvert::what() const
{
  return diagnostic::format("Could not convert from %1 to %2", this->source_type, this->dest_type);
}

const std::string CannotAccessMemberOfNonObject::message = "Cannot access member of non object type.";
const std::string NoSuchMember::message = "Object has no such member.";


const std::string InvalidTemplateArgument::message = "Invalid template argument.";
const std::string InvalidLiteralTemplateArgument::message = "Only integer and boolean literals can be used as template arguments.";
const std::string NonConstExprTemplateArgument::message = "Template arguments must be constant expressions.";
const std::string InvalidTemplateArgumentType::message = "This constant epression does not evaluate to an int or a bool.";

const std::string InvalidUseOfVirtualKeyword::message = "Invalid use of virtual keyword.";

const std::string AutoMustBeUsedWithAssignment::message = "'auto' can only be used with assignment initialization.";

const std::string CannotDeduceLambdaReturnType::message = "Cannot deduce lambda return type";

const std::string CallToDeletedFunction::message = "Call to deleted function.";

const std::string FunctionCannotBeDefaulted::message = "Function cannot be defaulted.";

const std::string ParentHasNoCopyConstructor::message = "Cannot generate defaulted copy constructor because parent has no copy constructor.";
const std::string ParentHasDeletedCopyConstructor::message = "Cannot generate defaulted copy constructor because parent copy constructor is deleted.";
const std::string DataMemberIsNotCopyable::message = "Cannot generate defaulted copy constructor because at least one data member is not copyable.";
const std::string ParentHasNoMoveConstructor::message = "Cannot generate defaulted move constructor because parent has no move constructor.";
const std::string ParentHasDeletedMoveConstructor::message = "Cannot generate defaulted move constructor because parent move constructor is deleted.";
const std::string DataMemberIsNotMovable::message = "Cannot generate defaulted move constructor because at least one data member is not movable.";

const std::string ParentHasNoAssignmentOperator::message = "Cannot generate defaulted assignment operator because parent has no assignment operator.";
const std::string ParentHasDeletedAssignmentOperator::message = "Cannot generate defaulted assignment operator because parent has a deleted assignment operator.";
const std::string DataMemberHasNoAssignmentOperator::message = "Cannot generate defaulted assignment operator because at least one data member has no assignment operator.";
const std::string DataMemberHasDeletedAssignmentOperator::message = "Cannot generate defaulted assignment operator because at least one data member has a deleted assignment operator.";

const std::string DataMemberIsReferenceAndCannotBeAssigned::message = "Cannot generate defaulted assignment operator because at least one data member is a reference.";

const std::string InvalidArgumentCountInDataMemberRefInit::message = "Only one value must be provided to initialize a data member of reference type.";
const std::string CannotInitializeNonConstRefDataMemberWithConst::message = "Cannot initialize a data member of non-const reference type with a const value.";
const std::string BadDataMemberRefInit::message = "Bad reference initialization of data member.";
const std::string EnumMemberCannotBeDefaultConstructed::message = "Data member of enumeration type cannot be default constructed.";
const std::string DataMemberHasNoDefaultConstructor::message = "Data member has no default constructor.";
const std::string DataMemberHasDeletedDefaultConstructor::message = "Data member has a deleted default constructor.";

const std::string InvalidCharacterLiteral::message = "A character literal must contain only one character.";

const std::string CouldNotFindValidLiteralOperator::message = "Could not find valid literal operator.";

const std::string UnknownTypeInBraceInitialization::message = "Unknown type in brace initialization";
const std::string NarrowingConversionInBraceInitialization::message = "Narrowing conversion in brace initialization";

} // namespace compiler

} // naemespace script
