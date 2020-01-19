// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/diagnosticmessage.h"

#include "script/accessspecifier.h"
#include "script/engine.h"
#include "script/initialization.h"
#include "script/operator.h"
#include "script/overloadresolution.h"
#include "script/typesystem.h"

#include "script/parser/token.h"
#include "script/parser/parsererrors.h"
#include "script/compiler/compilererrors.h"

#include <cstring>
#include <initializer_list>
#include <sstream>

namespace script
{

namespace diagnostic
{

class MessageDatabase
{
public:
  std::vector<std::string> messages;

public:
  MessageDatabase()
  {
    messages.resize(static_cast<int>(ErrorCode::LastCompilerError) + 1);
  }

public:
  void setMessage(ErrorCode c, std::string && mssg);
  void setMessages(std::initializer_list<std::pair<ErrorCode, std::string>> && list);

};

void MessageDatabase::setMessage(ErrorCode c, std::string && mssg)
{
  const int offset = static_cast<int>(c);
  messages[offset] = std::move(mssg);
}

void MessageDatabase::setMessages(std::initializer_list<std::pair<ErrorCode, std::string>> && list)
{
  for (auto & p : list)
    setMessage(p.first, std::string{ p.second });
}

static std::shared_ptr<MessageDatabase> build_message_database()
{
  auto ret = std::make_shared<MessageDatabase>();

  ret->setMessages({
    { ErrorCode::P_UnexpectedEndOfInput, "Unexpected end of input" },
    { ErrorCode::P_UnexpectedFragmentEnd, "Unexpected end of fragment" },
    { ErrorCode::P_UnexpectedToken, "expected %2 got %1||unexpected token %1" },
    { ErrorCode::P_InvalidEmptyBrackets, "Invalid empty brackets" },
    { ErrorCode::P_IllegalUseOfKeyword, "keyword not allowed in this context" },
    { ErrorCode::P_ExpectedIdentifier, "expected identifier got %1" },
    { ErrorCode::P_ExpectedUserDefinedName, "expected user-defined name got %1" },
    { ErrorCode::P_ExpectedLiteral, "expected literal name got %1" },
    { ErrorCode::P_ExpectedOperatorSymbol, "Expected 'operator' to be followed by operator symbol, got %1" },
    { ErrorCode::P_InvalidEmptyOperand, "invalid empty parentheses" },
    { ErrorCode::P_ExpectedDeclaration, "expected a declaration" },
    { ErrorCode::P_MissingConditionalColon, "Incomplete conditional expression (read '?' but could not find ':')" },
    { ErrorCode::P_CouldNotParseLambdaCapture, "Could not read lambda-capture" },
    { ErrorCode::P_ExpectedCurrentClassName, "expected current class name" },
    { ErrorCode::P_CouldNotReadType, "could not read type" },
  });

  ret->setMessages({
    { ErrorCode::RuntimeError, "RuntimeError:" },
    { ErrorCode::C_IllegalUseOfThis, "Illegal use of this" },
    { ErrorCode::C_ObjectHasNoDestructor, "Object has no destructor" },
    { ErrorCode::C_InvalidUseOfDelegatedConstructor, "No other member initializer may be present when using delegating constructors" },
    { ErrorCode::C_NotDataMember, "%1 does not name a data member" },
    { ErrorCode::C_InheritedDataMember, "cannot initialize inherited data member %1" },
    { ErrorCode::C_DataMemberAlreadyHasInitializer, "data member %1 already has an intializer" },
    { ErrorCode::C_NoDelegatingConstructorFound, "Could not find a delegate constructor" },
    { ErrorCode::C_CouldNotFindValidBaseConstructor, "Could not find valid base constructor" },
    { ErrorCode::C_InitializerListAsFirstArrayElement, "An initializer list cannot be used as the first element of an array" },
    { ErrorCode::C_ReturnStatementWithoutValue, "Cannot have return-statement without a value in function returning non-void" },
    { ErrorCode::C_ReturnStatementWithValue, "A function returning void cannot return a value" },
    { ErrorCode::C_ReferencesMustBeInitialized, "References must be initialized" },
    { ErrorCode::C_EnumerationsCannotBeDefaultConstructed, "Enumerations cannot be default constructed" },
    { ErrorCode::C_EnumerationsMustBeInitialized, "Variables of enumeration type must be initialized" },
    { ErrorCode::C_FunctionVariablesMustBeInitialized, "Variables of function-type must be initialized" },
    { ErrorCode::C_VariableCannotBeDefaultConstructed, "Class '%1' does not provide a default constructor" },
    { ErrorCode::C_ClassHasDeletedDefaultCtor, "Class '%1' has a deleted default constructor" },
    { ErrorCode::C_CouldNotResolveOperatorName, "Could not resolve operator name based on parameter count and operator symbol." },
    { ErrorCode::C_InvalidParamCountInOperatorOverload, "Invalid parameter count found in operator overload, expected %1 got %2" },
    { ErrorCode::C_OpOverloadMustBeDeclaredAsMember, "This operator can only be overloaded as a member" },
    { ErrorCode::C_InvalidTypeName, "%1 does not name a type" },
    { ErrorCode::C_DataMemberCannotBeAuto, "Data members cannot be declared 'auto'." },
    { ErrorCode::C_MissingStaticInitialization, "A static variable must be initialized." },
    { ErrorCode::C_InvalidStaticInitialization, "Static variables can only be initialized through assignment." },
    { ErrorCode::C_FailedToInitializeStaticVariable, "Failed to initialized static variable." },
    { ErrorCode::C_InvalidBaseClass, "Invalid base class." },
    { ErrorCode::C_InvalidUseOfDefaultArgument, "Cannot have a parameter without a default value after one was provided." },
    { ErrorCode::C_ArrayElementNotConvertible, "Could not convert element to array's element type." },
    { ErrorCode::C_ArraySubscriptOnNonObject, "Cannot perform array subscript on non object type." },
    { ErrorCode::C_CouldNotFindValidSubscriptOperator, "Could not find valid subscript operator." },
    { ErrorCode::C_CannotCaptureThis, "'this' cannot be captured outside of a member function." },
    { ErrorCode::C_UnknownCaptureName, "Could not capture any local variable with given name." },
    { ErrorCode::C_CannotCaptureNonCopyable, "Cannot capture by value a non copyable type." },
    { ErrorCode::C_SomeLocalsCannotBeCaptured, "Some local variables cannot be captured by value." },
    { ErrorCode::C_CannotCaptureByValueAndByRef, "Cannot capture both everything by reference and by value." },
    { ErrorCode::C_LambdaMustBeCaptureless, "A lambda must be captureless within this context." },
    { ErrorCode::C_CouldNotFindValidConstructor, "Could not find valid constructor." },
    { ErrorCode::C_CouldNotFindValidMemberFunction, "Could not find valid member function for call." },
    { ErrorCode::C_CouldNotFindValidOperator, "Could not find valid operator overload." },
    { ErrorCode::C_CouldNotFindValidCallOperator, "Could not find valid operator() overload for call." },
    { ErrorCode::C_AmbiguousFunctionName, "Name does not refer to a single function" },
    { ErrorCode::C_TemplateNamesAreNotExpressions, "Name refers to a template and cannot be used inside an expression" },
    { ErrorCode::C_TypeNameInExpression, "Name refers to a type and cannot be used inside an expression" },
    { ErrorCode::C_NamespaceNameInExpression, "Name refers to a namespace and cannot be used inside an expression" },
    { ErrorCode::C_TooManyArgumentInVariableInitialization, "Too many arguments provided in variable initialization." },
    { ErrorCode::C_TooManyArgumentInInitialization, "Too many arguments provided in initialization." },
    { ErrorCode::C_TooManyArgumentInReferenceInitialization, "More than one argument provided in reference initialization." },
    { ErrorCode::C_CouldNotConvert, "Could not convert from %1 to %2" },
    { ErrorCode::C_CouldNotFindCommonType, "Could not find common type of %1 and %2 in conditionnal expression" },
    { ErrorCode::C_CannotAccessMemberOfNonObject, "Cannot access member of non object type." },
    { ErrorCode::C_NoSuchMember, "Object has no such member." },
    { ErrorCode::C_InvalidTemplateArgument, "Invalid template argument." },
    { ErrorCode::C_InvalidLiteralTemplateArgument, "Only integer and boolean literals can be used as template arguments." },
    { ErrorCode::C_MissingNonDefaultedTemplateParameter, "Missing non-defaulted template parameter." },
    { ErrorCode::C_CouldNotFindPrimaryClassTemplate, "Could not find primary class template (must be declared in the same namespace)." },
    { ErrorCode::C_CouldNotFindPrimaryFunctionTemplate, "Could not find primary function template (must be declared in the same namespace)." },
    { ErrorCode::C_InvalidUseOfConstKeyword, "Invalid use of const keyword." },
    { ErrorCode::C_InvalidUseOfExplicitKeyword, "Invalid use of 'explicit' keyword." },
    { ErrorCode::C_InvalidUseOfStaticKeyword, "Invalid use of static keyword." },
    { ErrorCode::C_InvalidUseOfVirtualKeyword, "Invalid use of virtual keyword." },
    { ErrorCode::C_AutoMustBeUsedWithAssignment, "'auto' can only be used with assignment initialization." },
    { ErrorCode::C_CannotDeduceLambdaReturnType, "Cannot deduce lambda return type" },
    { ErrorCode::C_CallToDeletedFunction, "Call to deleted function." },
    { ErrorCode::C_FunctionCannotBeDefaulted, "Function cannot be defaulted." },
    { ErrorCode::C_ParentHasNoDefaultConstructor, "Cannot generate defaulted default constructor because parent has no default constructor." },
    { ErrorCode::C_ParentHasDeletedDefaultConstructor, "Cannot generate defaulted default constructor because parent default constructor is deleted." },
    { ErrorCode::C_ParentHasNoCopyConstructor, "Cannot generate defaulted copy constructor because parent has no copy constructor." },
    { ErrorCode::C_ParentHasDeletedCopyConstructor, "Cannot generate defaulted copy constructor because parent copy constructor is deleted." },
    { ErrorCode::C_DataMemberIsNotCopyable, "Cannot generate defaulted copy constructor because at least one data member is not copyable." },
    { ErrorCode::C_ParentHasDeletedMoveConstructor, "Cannot generate defaulted move constructor because parent move constructor is deleted." },
    { ErrorCode::C_DataMemberIsNotMovable, "Cannot generate defaulted move constructor because at least one data member is not movable." },
    { ErrorCode::C_ParentHasNoAssignmentOperator, "Cannot generate defaulted assignment operator because parent has no assignment operator." },
    { ErrorCode::C_ParentHasDeletedAssignmentOperator, "Cannot generate defaulted assignment operator because parent has a deleted assignment operator." },
    { ErrorCode::C_DataMemberHasNoAssignmentOperator, "Cannot generate defaulted assignment operator because at least one data member has no assignment operator." },
    { ErrorCode::C_DataMemberHasDeletedAssignmentOperator, "Cannot generate defaulted assignment operator because at least one data member has a deleted assignment operator." },
    { ErrorCode::C_DataMemberIsReferenceAndCannotBeAssigned, "Cannot generate defaulted assignment operator because at least one data member is a reference." },
    { ErrorCode::C_InvalidCharacterLiteral, "A character literal must contain only one character." },
    { ErrorCode::C_CouldNotFindValidLiteralOperator, "Could not find valid literal operator." },
    { ErrorCode::C_UnknownTypeInBraceInitialization, "Unknown type %1 in brace initialization" },
    { ErrorCode::C_NarrowingConversionInBraceInitialization, "Narrowing conversion from %1 to %2 in brace initialization" },
    { ErrorCode::C_NamespaceDeclarationCannotAppearAtThisLevel, "Namespace declarations cannot appear at this level" },
    { ErrorCode::C_ExpectedDeclaration, "Expected a declaration." },
    { ErrorCode::C_GlobalVariablesCannotBeAuto, "Global variables cannot be declared with auto." },
    { ErrorCode::C_GlobalVariablesMustBeInitialized, "Global variables must have an initializer." },
    { ErrorCode::C_GlobalVariablesMustBeAssigned, "Global variables must be initialized through assignment." },
    { ErrorCode::C_InaccessibleMember, "%1 is %2 within this context" },
    { ErrorCode::C_FriendMustBeAClass, "Friend must be a class" },
    { ErrorCode::C_UnknownModuleName, "Unknown module '%1'" },
    { ErrorCode::C_UnknownSubModuleName, "'%1' is not a submodule of '%2'" },
    { ErrorCode::C_ModuleImportationFailed, "Failed to import module '%1'\n%2" },
    { ErrorCode::C_InvalidNameInUsingDirective, "%1 does not name a namespace" },
    { ErrorCode::C_NoSuchCallee, "Callee was not declared in this scope" },
    { ErrorCode::C_LiteralOperatorNotInNamespace, "Literal operators can only appear at namespace level" },
  });

  return ret;
}

static const std::shared_ptr<MessageDatabase> gMessageDatabase = build_message_database();

inline static bool is_digit(const char c)
{
  return c >= '0' && c <= '9';
}

inline static bool is_letter(const char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline static bool is_alphanumeric(const char c)
{
  return is_letter(c) || is_digit(c);
}


struct placemarker_t
{
  size_t pos;
  size_t length;
  int value;
};

placemarker_t read_placemarker(const std::string & str, size_t pos)
{
  size_t num_start = pos + 1;
  size_t end = num_start;
  while (end < str.size() && is_digit(str[end]))
    ++end;

  if (end == num_start)
    return placemarker_t{ std::string::npos, 0, -1 };

  return placemarker_t{ pos, end - pos, std::stoi(str.substr(num_start, end - num_start)) };
}

placemarker_t find_lowest_placemarker(const std::string & str)
{
  std::size_t pos = str.find('%');
  if (pos == std::string::npos)
    return placemarker_t{ pos, 0, -1 };

  placemarker_t pm = read_placemarker(str, pos);
  pos = str.find('%', pos + 1);
  while (pos != std::string::npos)
  {
    placemarker_t new_pm = read_placemarker(str, pos);
    if (new_pm.value != -1 && new_pm.value < pm.value)
      pm = new_pm;
    pos = str.find('%', pos + 1);
  }

  return pm;
}

const std::string & format(const std::string & str)
{
  return str;
}

std::string format(std::string str, const std::string & a)
{
  placemarker_t pm = find_lowest_placemarker(str);
  if (pm.value == -1)
    return str;

  str.replace(str.begin() + pm.pos, str.begin() + pm.pos + pm.length, a);
  return str;
}

std::string format(std::string str, const std::string & arg1, const std::string & arg2)
{
  return format(format(str, arg1), arg2);
}

std::string format(std::string str, const std::string & arg1, const std::string & arg2, const std::string & arg3)
{
  return format(format(str, arg1, arg2), arg3);
}

std::string format(std::string str, const std::string & arg1, const std::string & arg2, const std::string & arg3, const std::string & arg4)
{
  return format(format(str, arg1, arg2, arg3), arg3, arg4);
}

DiagnosticMessage::DiagnosticMessage(Severity s)
  : mSeverity(s)
{

}

DiagnosticMessage::DiagnosticMessage(Severity s, std::error_code ec, SourceLocation loc, std::string text)
  : mSeverity(s),
    mCode(ec),
    mLocation(loc),
    mContent(std::move(text))
{

}

DiagnosticMessage::DiagnosticMessage(Severity s, std::error_code ec, std::string text)
  : mSeverity(s),
    mCode(ec),
    mContent(std::move(text))
{

}

void DiagnosticMessage::setSeverity(Severity sev)
{
  mSeverity = sev;
}

std::string DiagnosticMessage::message() const
{
  std::string result;

  switch (mSeverity)
  {
  case Info:
    result += "[info]";
    break;
  case Warning:
    result += "[warning]";
    break;
  case Error:
    result += "[error]";
    break;
  }

  if (!mCode)
  {
    /// TODO: do something with that ?
  }


  if (line() != std::numeric_limits<uint16_t>::max())
  {
    result += std::to_string(line());
    result += std::string{ ":" };
    if (column() != std::numeric_limits<uint16_t>::max())
      result += std::to_string(column()) + std::string{ ":" };
  }

  result += " ";

  result += mContent;

  return result;
}

const std::string & DiagnosticMessage::content() const
{
  return mContent;
}

void DiagnosticMessage::setContent(std::string str)
{
  mContent = std::move(str);
}

void DiagnosticMessage::setCode(std::error_code ec)
{
  mCode = ec;
}

void DiagnosticMessage::setLocation(const SourceLocation& loc)
{
  mLocation = loc;
}

MessageBuilder::MessageBuilder(Engine* e)
  : mEngine(e)
{

}

MessageBuilder::~MessageBuilder()
{

}

Verbosity MessageBuilder::verbosity() const
{
  return mVerbosity;
}

void MessageBuilder::setVerbosity(Verbosity ver)
{
  mVerbosity = ver;
}

void MessageBuilder::build(DiagnosticMessage& mssg, const parser::SyntaxError& ex)
{
  mssg.setCode(ex.errorCode());
  mssg.setLocation(ex.location);
  mssg.setContent(ex.errorCode().message());
}

void MessageBuilder::build(DiagnosticMessage& mssg, const compiler::CompilationFailure& ex)
{
  mssg.setCode(ex.errorCode());
  mssg.setLocation(ex.location);
  mssg.setContent(ex.errorCode().message());
}

std::string MessageBuilder::produce(const OverloadResolution& resol) const
{
  if (resol.success())
    return "Overload resolution succeeded";

  std::stringstream ss;

  if (!resol.ambiguousOverload().isNull())
  {
    ss << "Overload resolution failed because at least two candidates are not comparable or indistinguishable \n";
    ss << engine()->toString(resol.selectedOverload()) << "\n";
    ss << engine()->toString(resol.ambiguousOverload()) << "\n";
    return ss.str();
  }

  ss << "Overload resolution failed because no viable overload could be found \n";
  std::vector<Initialization> paraminits;
  for (const auto& f : resol.candidates())
  {
    auto status = resol.getViabilityStatus(f, &paraminits);
    ss << engine()->toString(f);
    if (status == OverloadResolution::IncorrectParameterCount)
      ss << "\n" << "Incorrect argument count, expects " << f.prototype().count() << " but " << resol.inputSize() << " were provided";
    else if (status == OverloadResolution::CouldNotConvertArgument)
      ss << "\n" << "Could not convert argument " << paraminits.size();
    ss << "\n";
  }
  return ss.str();
}

} // namespace diagnostic

} // namespace script
