// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/errors.h"
#include "script/compiler/compilererrors.h"

namespace script
{

namespace errors
{

class CompilerCategory : public std::error_category
{
public:

  const char* name() const noexcept override
  {
    return "compiler-category";
  }

  std::string message(int code) const override
  {
    switch (static_cast<CompilerError>(code))
    {
    case CompilerError::IllegalUseOfThis:
      return "illegal use of this";
    case CompilerError::ObjectHasNoDestructor:
      return "object has no destructor";
    case CompilerError::InvalidUseOfDelegatedConstructor:
      return "no other member initializer may be present when using delegating constructors";
    case CompilerError::NotDataMember:
      return "no data member with the given name";
    case CompilerError::InheritedDataMember:
      return "cannot initialize inherited data member";
    case CompilerError::DataMemberAlreadyHasInitializer:
      return "data member already has an intializer";
    case CompilerError::NoDelegatingConstructorFound:
      return "could not find a delegate constructor";
    case CompilerError::CouldNotFindValidBaseConstructor:
      return "could not find valid base constructor";
    case CompilerError::InitializerListAsFirstArrayElement:
      return "an initializer list cannot be used as the first element of an array";
    case CompilerError::ReturnStatementWithoutValue:
      return "cannot have return-statement without a value in function returning non-void";
    case CompilerError::ReturnStatementWithValue:
      return "a function returning void cannot return a value";
    case CompilerError::ReferencesMustBeInitialized:
      return "references must be initialized";
    case CompilerError::EnumerationsCannotBeDefaultConstructed:
      return "enumerations cannot be default constructed";
    case CompilerError::EnumerationsMustBeInitialized:
      return "variables of enumeration type must be initialized";
    case CompilerError::FunctionVariablesMustBeInitialized:
      return "variables of function-type must be initialized";
    case CompilerError::VariableCannotBeDefaultConstructed:
      return "class does not provide a default constructor";
    case CompilerError::ClassHasDeletedDefaultCtor:
      return "class has a deleted default constructor";
    case CompilerError::CouldNotResolveOperatorName:
      return "could not resolve operator name based on parameter count and operator symbol.";
    case CompilerError::InvalidParamCountInOperatorOverload:
      return "invalid parameter count found in operator overload";
    case CompilerError::OpOverloadMustBeDeclaredAsMember:
      return "this operator can only be overloaded as a member";
    case CompilerError::InvalidTypeName:
      return "identifier does not name a type";
    case CompilerError::DataMemberCannotBeAuto:
      return "data members cannot be declared 'auto'.";
    case CompilerError::MissingStaticInitialization:
      return "a static variable must be initialized.";
    case CompilerError::InvalidStaticInitialization:
      return "static variables can only be initialized through assignment.";
    case CompilerError::FailedToInitializeStaticVariable:
      return "failed to initialized static variable.";
    case CompilerError::InvalidBaseClass:
      return "invalid base class.";
    case CompilerError::InvalidUseOfDefaultArgument:
      return "cannot have a parameter without a default value after one was provided.";
    case CompilerError::ArrayElementNotConvertible:
      return "could not convert element to array's element type.";
    case CompilerError::ArraySubscriptOnNonObject:
      return "cannot perform array subscript on non object type.";
    case CompilerError::CouldNotFindValidSubscriptOperator:
      return "could not find valid subscript operator.";
    case CompilerError::CannotCaptureThis:
      return "'this' cannot be captured outside of a member function.";
    case CompilerError::UnknownCaptureName:
      return "could not capture any local variable with given name.";
    case CompilerError::CannotCaptureNonCopyable:
      return "cannot capture by value a non copyable type.";
    case CompilerError::SomeLocalsCannotBeCaptured:
      return "some local variables cannot be captured by value.";
    case CompilerError::CannotCaptureByValueAndByRef:
      return "cannot capture both everything by reference and by value.";
    case CompilerError::LambdaMustBeCaptureless:
      return "a lambda must be captureless within this context.";
    case CompilerError::CouldNotFindValidConstructor:
      return "could not find valid constructor.";
    case CompilerError::CouldNotFindValidMemberFunction:
      return "could not find valid member function for call.";
    case CompilerError::CouldNotFindValidOperator:
      return "could not find valid operator overload.";
    case CompilerError::CouldNotFindValidCallOperator:
      return "could not find valid operator() overload for call.";
    case CompilerError::AmbiguousFunctionName:
      return "name does not refer to a single function";
    case CompilerError::TemplateNamesAreNotExpressions:
      return "name refers to a template and cannot be used inside an expression";
    case CompilerError::TypeNameInExpression:
      return "name refers to a type and cannot be used inside an expression";
    case CompilerError::NamespaceNameInExpression:
      return "name refers to a namespace and cannot be used inside an expression";
    case CompilerError::TooManyArgumentInVariableInitialization:
      return "too many arguments provided in variable initialization.";
    case CompilerError::TooManyArgumentInInitialization:
      return "too many arguments provided in initialization.";
    case CompilerError::TooManyArgumentInReferenceInitialization:
      return "more than one argument provided in reference initialization.";
    case CompilerError::CouldNotConvert:
      return "conversion failed";
    case CompilerError::CouldNotFindCommonType:
      return "no common type in conditional expression";
    case CompilerError::CannotAccessMemberOfNonObject:
      return "cannot access member of non object type.";
    case CompilerError::NoSuchMember:
      return "object has no such member.";
    case CompilerError::InvalidTemplateArgument:
      return "invalid template argument.";
    case CompilerError::InvalidLiteralTemplateArgument:
      return "only integer and boolean literals can be used as template arguments.";
    case CompilerError::MissingNonDefaultedTemplateParameter:
      return "missing non-defaulted template parameter.";
    case CompilerError::CouldNotFindPrimaryClassTemplate:
      return "could not find primary class template (must be declared in the same namespace).";
    case CompilerError::CouldNotFindPrimaryFunctionTemplate:
      return "could not find primary function template (must be declared in the same namespace).";
    case CompilerError::InvalidUseOfConstKeyword:
      return "invalid use of const keyword.";
    case CompilerError::InvalidUseOfExplicitKeyword:
      return "invalid use of 'explicit' keyword.";
    case CompilerError::InvalidUseOfStaticKeyword:
      return "invalid use of static keyword.";
    case CompilerError::InvalidUseOfVirtualKeyword:
      return "invalid use of virtual keyword.";
    case CompilerError::AutoMustBeUsedWithAssignment:
      return "'auto' can only be used with assignment initialization.";
    case CompilerError::CannotDeduceLambdaReturnType:
      return "cannot deduce lambda return type";
    case CompilerError::CallToDeletedFunction:
      return "call to deleted function.";
    case CompilerError::FunctionCannotBeDefaulted:
      return "function cannot be defaulted.";
    case CompilerError::ParentHasNoDefaultConstructor:
      return "cannot generate defaulted default constructor because parent has no default constructor.";
    case CompilerError::ParentHasDeletedDefaultConstructor:
      return "cannot generate defaulted default constructor because parent default constructor is deleted.";
    case CompilerError::ParentHasNoCopyConstructor:
      return "cannot generate defaulted copy constructor because parent has no copy constructor.";
    case CompilerError::ParentHasDeletedCopyConstructor:
      return "cannot generate defaulted copy constructor because parent copy constructor is deleted.";
    case CompilerError::DataMemberIsNotCopyable:
      return "cannot generate defaulted copy constructor because at least one data member is not copyable.";
    case CompilerError::ParentHasDeletedMoveConstructor:
      return "cannot generate defaulted move constructor because parent move constructor is deleted.";
    case CompilerError::DataMemberIsNotMovable:
      return "cannot generate defaulted move constructor because at least one data member is not movable.";
    case CompilerError::ParentHasNoAssignmentOperator:
      return "cannot generate defaulted assignment operator because parent has no assignment operator.";
    case CompilerError::ParentHasDeletedAssignmentOperator:
      return "cannot generate defaulted assignment operator because parent has a deleted assignment operator.";
    case CompilerError::DataMemberHasNoAssignmentOperator:
      return "cannot generate defaulted assignment operator because at least one data member has no assignment operator.";
    case CompilerError::DataMemberHasDeletedAssignmentOperator:
      return "cannot generate defaulted assignment operator because at least one data member has a deleted assignment operator.";
    case CompilerError::DataMemberIsReferenceAndCannotBeAssigned:
      return "cannot generate defaulted assignment operator because at least one data member is a reference.";
    case CompilerError::InvalidCharacterLiteral:
      return "a character literal must contain only one character.";
    case CompilerError::CouldNotFindValidLiteralOperator:
      return "could not find valid literal operator.";
    case CompilerError::UnknownTypeInBraceInitialization:
      return "unknown type in brace initialization";
    case CompilerError::NarrowingConversionInBraceInitialization:
      return "narrowing conversion in brace initialization";
    case CompilerError::NamespaceDeclarationCannotAppearAtThisLevel:
      return "namespace declarations cannot appear at this level";
    case CompilerError::ExpectedDeclaration:
      return "expected a declaration.";
    case CompilerError::GlobalVariablesCannotBeAuto:
      return "global variables cannot be declared with auto.";
    case CompilerError::GlobalVariablesMustBeInitialized:
      return "global variables must have an initializer.";
    case CompilerError::GlobalVariablesMustBeAssigned:
      return "global variables must be initialized through assignment.";
    case CompilerError::InaccessibleMember:
      return "member is not accessible within this context";
    case CompilerError::FriendMustBeAClass:
      return "friend must be a class";
    case CompilerError::UnknownModuleName:
      return "unknown module name";
    case CompilerError::UnknownSubModuleName:
      return "unknown submodule name";
    case CompilerError::ModuleImportationFailed:
      return "failed to import module";
    case CompilerError::InvalidNameInUsingDirective:
      return "identifier does not name a namespace";
    case CompilerError::NoSuchCallee:
      return "callee was not declared in this scope";
    case CompilerError::LiteralOperatorNotInNamespace:
      return "literal operators can only appear at namespace level";
    default:
      return "unknown compiler error";
    }
  }
};

const std::error_category& compiler_category() noexcept
{
  static CompilerCategory static_instance = {};
  return static_instance;
}

} // namespace errors

namespace compiler
{

CompilerErrorData::~CompilerErrorData()
{

}

} // namespace compiler

} // namespace script

