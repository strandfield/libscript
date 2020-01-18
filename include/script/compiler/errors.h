// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_ERRORS_H
#define LIBSCRIPT_COMPILER_ERRORS_H

#include <system_error>

namespace script
{

namespace errors
{

const std::error_category& compiler_category() noexcept;

} // namespace errors

enum class CompilerError {
  SyntaxError = 1,
  IllegalUseOfThis,
  ObjectHasNoDestructor,
  InvalidUseOfDelegatedConstructor,
  NotDataMember,
  InheritedDataMember,
  DataMemberAlreadyHasInitializer,
  NoDelegatingConstructorFound,
  CouldNotFindValidBaseConstructor,
  InitializerListAsFirstArrayElement,
  ReturnStatementWithoutValue,
  ReturnStatementWithValue,
  ReferencesMustBeInitialized,
  EnumerationsCannotBeDefaultConstructed,
  EnumerationsMustBeInitialized,
  FunctionVariablesMustBeInitialized,
  VariableCannotBeDefaultConstructed,
  ClassHasDeletedDefaultCtor,
  CouldNotResolveOperatorName,
  InvalidParamCountInOperatorOverload,
  OpOverloadMustBeDeclaredAsMember,
  InvalidTypeName,
  DataMemberCannotBeAuto,
  MissingStaticInitialization,
  InvalidStaticInitialization,
  FailedToInitializeStaticVariable,
  InvalidBaseClass,
  InvalidUseOfDefaultArgument,
  ArrayElementNotConvertible,
  ArraySubscriptOnNonObject,
  CouldNotFindValidSubscriptOperator,
  CannotCaptureThis,
  UnknownCaptureName,
  CannotCaptureNonCopyable,
  SomeLocalsCannotBeCaptured,
  CannotCaptureByValueAndByRef,
  LambdaMustBeCaptureless,
  CouldNotFindValidConstructor,
  CouldNotFindValidMemberFunction,
  CouldNotFindValidOperator,
  CouldNotFindValidCallOperator,
  AmbiguousFunctionName,
  TemplateNamesAreNotExpressions,
  TypeNameInExpression,
  NamespaceNameInExpression,
  TooManyArgumentInVariableInitialization,
  TooManyArgumentInInitialization,
  TooManyArgumentInReferenceInitialization,
  CouldNotConvert,
  CouldNotFindCommonType,
  CannotAccessMemberOfNonObject,
  NoSuchMember,
  InvalidTemplateArgument,
  InvalidLiteralTemplateArgument,
  MissingNonDefaultedTemplateParameter,
  CouldNotFindPrimaryClassTemplate,
  CouldNotFindPrimaryFunctionTemplate,
  InvalidUseOfConstKeyword,
  InvalidUseOfExplicitKeyword,
  InvalidUseOfStaticKeyword,
  InvalidUseOfVirtualKeyword,
  AutoMustBeUsedWithAssignment,
  CannotDeduceLambdaReturnType,
  CallToDeletedFunction,
  FunctionCannotBeDefaulted,
  ParentHasNoDefaultConstructor,
  ParentHasDeletedDefaultConstructor,
  ParentHasNoCopyConstructor,
  ParentHasDeletedCopyConstructor,
  DataMemberIsNotCopyable,
  ParentHasDeletedMoveConstructor,
  DataMemberIsNotMovable,
  ParentHasNoAssignmentOperator,
  ParentHasDeletedAssignmentOperator,
  DataMemberHasNoAssignmentOperator,
  DataMemberHasDeletedAssignmentOperator,
  DataMemberIsReferenceAndCannotBeAssigned,
  InvalidCharacterLiteral,
  CouldNotFindValidLiteralOperator,
  UnknownTypeInBraceInitialization,
  NarrowingConversionInBraceInitialization,
  NamespaceDeclarationCannotAppearAtThisLevel,
  ExpectedDeclaration,
  GlobalVariablesCannotBeAuto,
  GlobalVariablesMustBeInitialized,
  GlobalVariablesMustBeAssigned,
  InaccessibleMember,
  FriendMustBeAClass,
  UnknownModuleName,
  UnknownSubModuleName,
  ModuleImportationFailed,
  InvalidNameInUsingDirective,
  NoSuchCallee,
  LiteralOperatorNotInNamespace,
};

} // namespace script

namespace std
{

template<> struct is_error_code_enum<script::CompilerError> : std::true_type { };

inline std::error_code make_error_code(script::CompilerError e) noexcept
{
  return std::error_code(static_cast<int>(e), script::errors::compiler_category());
}

} // namespace std

#endif // LIBSCRIPT_COMPILER_ERRORS_H
