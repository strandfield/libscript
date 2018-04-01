// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_ERRORS_H
#define LIBSCRIPT_COMPILER_ERRORS_H

#include <string>

namespace script
{

namespace compiler
{

class CompilerException
{
public:
  int line;
  int column;

  CompilerException();
  CompilerException(int l, int c);
  CompilerException(const CompilerException &) = default;
  virtual ~CompilerException() = default;
  
  virtual std::string what() const = 0;
};

#define DECLARE_ERROR(name) class name : public CompilerException { \
public: \
  name() = default; \
  name(int l, int c) : CompilerException(l, c) {} \
  ~name() = default; \
  static const std::string message; \
  std::string what() const override { return message; } \
}


#define DECLARE_ERROR_2(name, parent) class name : public parent { \
public: \
  ~name() = default; \
  static const std::string message; \
  std::string what() const override { return message; } \
}

class OperatorOverloadCompilationException : public CompilerException {};

DECLARE_ERROR(IllegalUseOfThis);

DECLARE_ERROR(ObjectHasNoDestructor);
DECLARE_ERROR(InvalidUseOfDelegatedConstructor);
DECLARE_ERROR(NotDataMember);
DECLARE_ERROR(InheritedDataMember);
DECLARE_ERROR(DataMemberAlreadyHasInitializer);
DECLARE_ERROR(NoDelegatingConstructorFound);
DECLARE_ERROR(CouldNotFindValidBaseConstructor);
DECLARE_ERROR(InitializerListAsFirstArrayElement);

DECLARE_ERROR(ReturnStatementWithoutValue);
DECLARE_ERROR(ReturnStatementWithValue);

DECLARE_ERROR(ReferencesMustBeInitialized);
DECLARE_ERROR(EnumerationsMustBeInitialized);
DECLARE_ERROR(FunctionVariablesMustBeInitialized);
DECLARE_ERROR(VariableCannotBeDefaultConstructed);
DECLARE_ERROR(VariableCannotBeDestroyed);

DECLARE_ERROR_2(TooManyArgumentsInOperatorOverload, OperatorOverloadCompilationException);
DECLARE_ERROR_2(InvalidParamCountInOperatorOverload, OperatorOverloadCompilationException);
DECLARE_ERROR_2(OpOverloadMustBeDeclaredAsMember, OperatorOverloadCompilationException);

class InvalidTypeName : public CompilerException {
public:
  std::string name;

  InvalidTypeName(int l, int c, const std::string & n);
  std::string what() const override;
};

DECLARE_ERROR(DeclarationProcessingError);

DECLARE_ERROR(DataMemberCannotBeAuto);
DECLARE_ERROR(MissingStaticInitialization);
DECLARE_ERROR(InvalidStaticInitialization);

DECLARE_ERROR(InvalidBaseClass);

DECLARE_ERROR(InvalidUseOfDefaultArgument);

DECLARE_ERROR(ArrayElementNotConvertible);
DECLARE_ERROR(ArraySubscriptOnNonObject);
DECLARE_ERROR(CouldNotFindValidSubscriptOperator);


DECLARE_ERROR(CannotCaptureThis);
DECLARE_ERROR(UnknownCaptureName);
DECLARE_ERROR(CannotCaptureNonCopyable);
DECLARE_ERROR(SomeLocalsCannotBeCaptured);
DECLARE_ERROR(CannotCaptureByValueAndByRef);
DECLARE_ERROR(LambdaMustBeCaptureless);

DECLARE_ERROR(CouldNotFindValidConstructor);
DECLARE_ERROR(CouldNotFindValidMemberFunction);
DECLARE_ERROR(CouldNotFindValidOperator);
DECLARE_ERROR(CouldNotFindValidOverload);
DECLARE_ERROR(CouldNotFindValidCallOperator);

DECLARE_ERROR(AmbiguousFunctionName);
DECLARE_ERROR(TemplateNamesAreNotExpressions);
DECLARE_ERROR(TypeNameInExpression);
DECLARE_ERROR(NamespaceNameInExpression);

DECLARE_ERROR(TooManyArgumentInVariableInitialization);
DECLARE_ERROR(TooManyArgumentInReferenceInitialization);
DECLARE_ERROR(TooManyArgumentsInMemberInitialization);


class CouldNotConvert : public CompilerException {
public:
  std::string source_type;
  std::string dest_type;

  CouldNotConvert(int l, int c, const std::string & src, const std::string & dest);
  std::string what() const override;
};


DECLARE_ERROR(CannotAccessMemberOfNonObject);
DECLARE_ERROR(NoSuchMember);

DECLARE_ERROR(InvalidTemplateArgument);
DECLARE_ERROR(InvalidLiteralTemplateArgument);
DECLARE_ERROR(NonConstExprTemplateArgument);
DECLARE_ERROR(InvalidTemplateArgumentType);

DECLARE_ERROR(InvalidUseOfVirtualKeyword);

DECLARE_ERROR(AutoMustBeUsedWithAssignment);

DECLARE_ERROR(CannotDeduceLambdaReturnType);

DECLARE_ERROR(CallToDeletedFunction);
DECLARE_ERROR(FunctionCannotBeDefaulted);

DECLARE_ERROR(ParentHasNoCopyConstructor);
DECLARE_ERROR(ParentHasDeletedCopyConstructor);
DECLARE_ERROR(DataMemberIsNotCopyable);
DECLARE_ERROR(ParentHasNoMoveConstructor);
DECLARE_ERROR(ParentHasDeletedMoveConstructor);
DECLARE_ERROR(DataMemberIsNotMovable);

DECLARE_ERROR(ParentHasNoAssignmentOperator);
DECLARE_ERROR(ParentHasDeletedAssignmentOperator);
DECLARE_ERROR(DataMemberHasNoAssignmentOperator);
DECLARE_ERROR(DataMemberHasDeletedAssignmentOperator);
DECLARE_ERROR(DataMemberIsReferenceAndCannotBeAssigned);

DECLARE_ERROR(InvalidArgumentCountInDataMemberRefInit);
DECLARE_ERROR(CannotInitializeNonConstRefDataMemberWithConst);
DECLARE_ERROR(BadDataMemberRefInit);
DECLARE_ERROR(EnumMemberCannotBeDefaultConstructed);
DECLARE_ERROR(DataMemberHasNoDefaultConstructor);
DECLARE_ERROR(DataMemberHasDeletedDefaultConstructor);

DECLARE_ERROR(InvalidCharacterLiteral);

DECLARE_ERROR(CouldNotFindValidLiteralOperator);

DECLARE_ERROR(UnknownTypeInBraceInitialization);
DECLARE_ERROR(NarrowingConversionInBraceInitialization);


class NotImplementedError : public CompilerException {
public: 
  NotImplementedError(const std::string & mssg) : message(mssg) {}
  NotImplementedError(std::string && mssg) : message(std::move(mssg)) {}
  std::string what() const override { return this->message; } 

  std::string message;
};

#undef DECLARE_ERROR
#undef DECLARE_ERROR_2


} // namespace compiler

} // namespace script


#endif // LIBSCRIPT_COMPILER_ERRORS_H
